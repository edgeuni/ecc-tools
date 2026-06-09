#include "ifill_runner.h"

#include <algorithm>
#include <iostream>
#include <stdexcept>

#include "IdbDesign.h"
#include "IdbGeometry.h"
#include "IdbLayout.h"
#include "IdbLayer.h"
#include "idm.h"
#include "ifill_core.h"
#include "ifill_rule_parser.h"
#include "vec_grid_info.h"
#include "vec_layout_dm.h"

namespace ifill {
namespace {

int32_t getDbuPerMicron(idb::IdbLayout* layout, idb::IdbDesign* design)
{
  if (design != nullptr && design->get_units() != nullptr && design->get_units()->get_micron_dbu() > 0) {
    return design->get_units()->get_micron_dbu();
  }
  if (layout != nullptr && layout->get_units() != nullptr && layout->get_units()->get_micron_dbu() > 0) {
    return layout->get_units()->get_micron_dbu();
  }
  throw std::runtime_error("ifill requires valid DBU units from iDB");
}

Rect toRect(idb::IdbRect* rect)
{
  return {rect->get_low_x(), rect->get_low_y(), rect->get_high_x(), rect->get_high_y()};
}

Rect defaultFillBounds(idb::IdbLayout* layout)
{
  if (layout->get_core() != nullptr && layout->get_core()->get_bounding_box() != nullptr) {
    return toRect(layout->get_core()->get_bounding_box());
  }
  if (layout->get_die() != nullptr && layout->get_die()->get_bounding_box() != nullptr) {
    return toRect(layout->get_die()->get_bounding_box());
  }
  throw std::runtime_error("ifill requires a core or die area");
}

Rect parseArea(const std::vector<int32_t>& area, idb::IdbLayout* layout)
{
  if (area.empty()) {
    return defaultFillBounds(layout);
  }
  if (area.size() != 4) {
    throw std::runtime_error("ifill -area requires exactly four DBU coordinates");
  }
  return {area[0], area[1], area[2], area[3]};
}

std::vector<Rect> collectOccupiedRects(ivec::VecLayout& vec_layout, const std::string& layer_name)
{
  std::vector<Rect> occupied;
  const int layer_order = vec_layout.findLayerId(layer_name);
  auto* layout_layer = vec_layout.get_layout_layers().findLayoutLayer(layer_order);
  if (layout_layer == nullptr) {
    return occupied;
  }

  const int32_t half_x = std::max(ivec::gridInfoInst.x_step / 2, 1);
  const int32_t half_y = std::max(ivec::gridInfoInst.y_step / 2, 1);
  for (auto* node : layout_layer->get_grid().get_all_nodes()) {
    if (node == nullptr || node->get_node_data() == nullptr) {
      continue;
    }
    const int32_t x = static_cast<int32_t>(node->get_x());
    const int32_t y = static_cast<int32_t>(node->get_y());
    occupied.push_back({x - half_x, y - half_y, x + half_x, y + half_y});
  }

  return occupied;
}

void writeLayerFills(idb::IdbDesign* design, idb::IdbLayer* layer, const std::vector<Rect>& fills)
{
  if (fills.empty()) {
    return;
  }
  auto* fill_layer = design->get_fill_list()->add_fill_layer(layer);
  for (const auto& rect : fills) {
    fill_layer->add_rect(rect.lx, rect.ly, rect.ux, rect.uy);
  }
}

}  // namespace

int32_t IFillRunner::run(const IFillRunOptions& options) const
{
  auto* layout = dmInst->get_idb_layout();
  auto* design = dmInst->get_idb_design();
  if (layout == nullptr || design == nullptr) {
    throw std::runtime_error("ifill requires initialized iDB layout and design");
  }
  if (design->get_fill_list() == nullptr) {
    throw std::runtime_error("ifill requires an initialized iDB fill list");
  }
  if (options.reset_fill) {
    design->get_fill_list()->reset();
  }

  ivec::VecLayoutDataManager layout_dm;
  layout_dm.buildLayoutData(false);
  auto& vec_layout = layout_dm.get_layout();

  const int32_t dbu_per_micron = getDbuPerMicron(layout, design);
  auto rules = loadLayerFillRules(options.rule_file, dbu_per_micron);
  const Rect fill_bounds = parseArea(options.area, layout);

  int32_t inserted = 0;
  MetalFillGenerator generator;
  for (auto& rule : rules) {
    auto* layer = layout->get_layers()->find_layer(rule.layer_name);
    if (layer == nullptr || !layer->is_routing()) {
      std::cout << "ifill skip unknown or non-routing layer: " << rule.layer_name << std::endl;
      continue;
    }

    auto* routing_layer = dynamic_cast<idb::IdbLayerRouting*>(layer);
    rule.horizontal = routing_layer == nullptr ? true : routing_layer->is_horizontal();

    LayerFillInput input;
    input.bounds = fill_bounds;
    input.rule = rule;
    input.occupied = collectOccupiedRects(vec_layout, rule.layer_name);

    const auto fills = generator.generate(input);
    writeLayerFills(design, layer, fills);
    inserted += static_cast<int32_t>(fills.size());
  }

  return inserted;
}

}  // namespace ifill
