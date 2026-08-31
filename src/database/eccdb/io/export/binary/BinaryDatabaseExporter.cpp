#include "export/binary/BinaryDatabaseExporter.h"

#include "design/DesignDatabase.h"
#include "library/LibraryDatabase.h"
#include "persistence/binary/BinaryFormat.h"
#include "persistence/binary/BinaryPayload.h"
#include "persistence/binary/BinarySchema.h"
#include "tech/TechDatabase.h"

namespace eccdb {
void BinaryDatabaseExporter::saveTech(const std::filesystem::path& path, const TechDatabase& database)
{
  binary_detail::BinaryFileHeader header;
  header.kind = binary_detail::BinaryDatabaseKind::kTech;
  header.schema_version = binary_detail::kTechSchemaVersion;
  binary_detail::writeBinaryFile(path, header,
                                 [&database](std::ostream& output) { binary_detail::writeTechPayload(output, database); });
}

void BinaryDatabaseExporter::saveLibrary(const std::filesystem::path& path, const LibraryDatabase& library)
{
  binary_detail::BinaryFileHeader header;
  header.kind = binary_detail::BinaryDatabaseKind::kLibrary;
  header.schema_version = binary_detail::kLibrarySchemaVersion;
  binary_detail::writeBinaryFile(path, header,
                                 [&library](std::ostream& output) { binary_detail::writeLibraryPayload(output, library); });
}

void BinaryDatabaseExporter::saveDesign(const std::filesystem::path& path, const DesignDatabase& design)
{
  binary_detail::BinaryFileHeader header;
  header.kind = binary_detail::BinaryDatabaseKind::kDesign;
  header.schema_version = binary_detail::kDesignSchemaVersion;
  binary_detail::writeBinaryFile(path, header,
                                 [&design](std::ostream& output) { binary_detail::writeDesignPayload(output, design); });
}

}  // namespace eccdb
