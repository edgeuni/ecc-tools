#pragma once

#include <iosfwd>

namespace eccdb {

class DesignDatabase;
class LibraryDatabase;
class TechDatabase;

namespace binary_detail {

void writeTechPayload(std::ostream& output, const TechDatabase& database);
void writeLibraryPayload(std::ostream& output, const LibraryDatabase& database);
void writeDesignPayload(std::ostream& output, const DesignDatabase& database);

}  // namespace binary_detail
}  // namespace eccdb
