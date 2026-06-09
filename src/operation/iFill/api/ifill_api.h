#pragma once

#include <cstdint>
#include <string>
#include <vector>

#define ifillApiInst ifill::IFillApi::getInstance()

namespace ifill {

class IFillApi
{
 public:
  static IFillApi* getInstance()
  {
    if (_instance == nullptr) {
      _instance = new IFillApi();
    }
    return _instance;
  }

  static void destroyInst()
  {
    delete _instance;
    _instance = nullptr;
  }

  int32_t runMetalFill(const std::string& rule_file, const std::vector<int32_t>& area = {}, bool reset_fill = false);

 private:
  static IFillApi* _instance;

  IFillApi() = default;
  ~IFillApi() = default;
  IFillApi(const IFillApi&) = delete;
  IFillApi(IFillApi&&) = delete;
  IFillApi& operator=(const IFillApi&) = delete;
  IFillApi& operator=(IFillApi&&) = delete;
};

}  // namespace ifill
