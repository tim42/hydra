
#include <magic.h>

#include <ntools/id/string_id.hpp>
#include <ntools/tracy.hpp>

#include "mimetype.hpp"

static magic_t get_mime_magic()
{
  thread_local magic_t token = nullptr;
  if (!token)
  {
    token = magic_open(MAGIC_MIME_TYPE);
    magic_load(token, nullptr);
  }
  return token;
}

const char* neam::resources::mime::bad_idea::get_mimetype(const std::filesystem::path& path)
{
  TRACY_SCOPED_ZONE;
  magic_t token = get_mime_magic();
  return magic_file(token, path.c_str());
}

neam::id_t neam::resources::mime::bad_idea::get_mimetype_id(const std::filesystem::path& path)
{
  const char* const mime = get_mimetype(path);
  return string_id::_runtime_build_from_string(mime, strlen(mime));
}

const char* neam::resources::mime::get_mimetype(const void* data, size_t len)
{
  TRACY_SCOPED_ZONE;
  magic_t token = get_mime_magic();
  return magic_buffer(token, data, len);
}

neam::id_t neam::resources::mime::get_mimetype_id(const void* data, size_t len)
{
  const char* const mime = get_mimetype(data, len);
  return string_id::_runtime_build_from_string(mime, strlen(mime));
}
