// This file is part of PyCPL the ESO CPL Python language bindings
// Copyright (C) 2020-2024 European Southern Observatory
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.

#include "cpldfs/dfs.hpp"

#include "cplcore/error.hpp"

namespace cpl
{
namespace dfs
{
void
sign_products(std::shared_ptr<cpl::ui::FrameSet> set, bool md5, bool checksum)
{
  unsigned int flags = CPL_DFS_SIGNATURE_NONE;  // Start at bitwise 0

  if (md5) {
    flags |= CPL_DFS_SIGNATURE_DATAMD5;  // Add the datamd5 flag
  }
  if (checksum) {
    flags |= CPL_DFS_SIGNATURE_CHECKSUM;  // Add the checksum flag
  }
  cpl_frameset* cpl_form = set->to_cpl();

  cpl::core::Error::throw_errors_with(cpl_dfs_sign_products, cpl_form, flags);
  cpl_frameset_delete(cpl_form);  // Delete duplicate frameset
}

void
update_product_header(std::shared_ptr<cpl::ui::FrameSet> set)
{
  cpl_frameset* cpl_form = set->to_cpl();

  cpl::core::Error::throw_errors_with(cpl_dfs_update_product_header, cpl_form);
  cpl_frameset_delete(cpl_form);  // Delete duplicate frameset
}

void
setup_product_header(std::shared_ptr<cpl::core::PropertyList> header,
                     std::shared_ptr<cpl::ui::Frame> product_frame,
                     std::shared_ptr<cpl::ui::FrameSet> framelist,
                     std::shared_ptr<cpl::ui::ParameterList> parlist,
                     const std::string& recid, const std::string& pipeline_id,
                     const std::string& dictionary_id,
                     std::shared_ptr<cpl::ui::Frame> inherit_frame)
{
  cpl_propertylist* header_ptr = cpl_propertylist_duplicate(
      header->ptr().get());  // Duplicate for modification

  const cpl_frame* product_frame_ptr = product_frame->ptr();

  cpl_frameset* framelist_ptr = framelist->to_cpl();  // Creates duplicate

  const cpl_frame* inherit_frame_ptr =
      (inherit_frame != NULL) ? inherit_frame->ptr() : NULL;

  cpl::core::Error::throw_errors_with(
      cpl_dfs_setup_product_header, header_ptr, product_frame_ptr,
      framelist_ptr, parlist->ptr().get(), recid.c_str(), pipeline_id.c_str(),
      dictionary_id.c_str(), inherit_frame_ptr);
  cpl::core::Error::throw_errors_with(cpl_frameset_delete, framelist_ptr);
  cpl::core::PropertyList new_proplist(header_ptr);
  // Clear and use new propertylist
  header->clear();
  header->append(new_proplist);
}

void
save_propertylist(std::shared_ptr<cpl::ui::FrameSet> allframes,
                  std::shared_ptr<cpl::ui::ParameterList> parlist,
                  std::shared_ptr<cpl::ui::FrameSet> usedframes,
                  const std::string& recipe,
                  std::shared_ptr<cpl::core::PropertyList> applist,
                  const std::string& pipe_id, const std::string& filename,
                  std::shared_ptr<cpl::core::PropertyList> header,  // Optional
                  std::shared_ptr<cpl::ui::Frame> inherit,
                  std::optional<std::string> remregexp)  // Optional
{
  cpl_frameset* allframes_ptr = allframes->to_cpl();  // Creates duplicate

  std::unique_ptr<struct _cpl_propertylist_, void (*)(cpl_propertylist*)>
      header_ptr = (header != NULL)
                       ? header->ptr()
                       : std::unique_ptr<struct _cpl_propertylist_,
                                         void (*)(cpl_propertylist*)>(
                             NULL, NULL);  // TODO: Gotta deal with the unique
                                           // pointer deallocating immediately

  cpl_frameset* usedframes_ptr = usedframes->to_cpl();  // Creates duplicate

  const cpl_frame* inherit_ptr = (inherit != NULL) ? inherit->ptr() : NULL;

  const char* remregexp_ptr =
      (remregexp.has_value()) ? remregexp.value().c_str() : NULL;

  cpl::core::Error::throw_errors_with(
      cpl_dfs_save_propertylist, allframes_ptr, header_ptr.get(),
      parlist->ptr().get(), usedframes_ptr, inherit_ptr, recipe.c_str(),
      applist->ptr().get(), remregexp_ptr, pipe_id.c_str(), filename.c_str());
  cpl::core::Error::throw_errors_with(cpl_frameset_delete, allframes_ptr);
  cpl::core::Error::throw_errors_with(cpl_frameset_delete, usedframes_ptr);
}

void
save_paf(const std::string& instrume, const std::string& recipe,
         std::shared_ptr<cpl::core::PropertyList> paflist,
         const std::string& filename)
{
  cpl::core::Error::throw_errors_with(cpl_dfs_save_paf, instrume.c_str(),
                                      recipe.c_str(), paflist->ptr().get(),
                                      filename.c_str());
}

void
save_imagelist(std::shared_ptr<cpl::ui::FrameSet> allframes,
               std::shared_ptr<cpl::ui::ParameterList> parlist,
               std::shared_ptr<cpl::ui::FrameSet> usedframes,
               std::shared_ptr<cpl::core::ImageList> imagelist, cpl_type type,
               const std::string& recipe,
               std::shared_ptr<cpl::core::PropertyList> applist,
               const std::string& pipe_id, const std::string& filename,
               std::shared_ptr<cpl::core::PropertyList> header,
               std::shared_ptr<cpl::ui::Frame> inherit,
               std::optional<std::string> remregexp)
{
  cpl_frameset* allframes_ptr = allframes->to_cpl();  // Creates duplicate

  std::unique_ptr<struct _cpl_propertylist_, void (*)(cpl_propertylist*)>
      header_ptr = (header != NULL)
                       ? header->ptr()
                       : std::unique_ptr<struct _cpl_propertylist_,
                                         void (*)(cpl_propertylist*)>(
                             NULL, NULL);  // TODO: Gotta deal with the unique
                                           // pointer deallocating immediately

  cpl_frameset* usedframes_ptr = usedframes->to_cpl();  // Creates duplicate

  const cpl_frame* inherit_ptr = (inherit != NULL) ? inherit->ptr() : NULL;

  const char* remregexp_ptr =
      (remregexp.has_value()) ? remregexp.value().c_str() : NULL;

  cpl::core::Error::throw_errors_with(
      cpl_dfs_save_imagelist, allframes_ptr, header_ptr.get(),
      parlist->ptr().get(), usedframes_ptr, inherit_ptr, imagelist->ptr(), type,
      recipe.c_str(), applist->ptr().get(), remregexp_ptr, pipe_id.c_str(),
      filename.c_str());
  cpl::core::Error::throw_errors_with(cpl_frameset_delete, allframes_ptr);
  cpl::core::Error::throw_errors_with(cpl_frameset_delete, usedframes_ptr);
}

void
save_image(std::shared_ptr<cpl::ui::FrameSet> allframes,
           std::shared_ptr<cpl::ui::ParameterList> parlist,
           std::shared_ptr<cpl::ui::FrameSet> usedframes,
           std::shared_ptr<cpl::core::ImageBase> image,
           const std::string& recipe,
           std::shared_ptr<cpl::core::PropertyList> applist,
           const std::string& pipe_id, const std::string& filename,
           std::shared_ptr<cpl::core::PropertyList> header,
           std::shared_ptr<cpl::ui::Frame> inherit,
           std::optional<std::string> remregexp)
{
  cpl_frameset* allframes_ptr = allframes->to_cpl();  // Creates duplicate

  std::unique_ptr<struct _cpl_propertylist_, void (*)(cpl_propertylist*)>
      header_ptr = (header != NULL)
                       ? header->ptr()
                       : std::unique_ptr<struct _cpl_propertylist_,
                                         void (*)(cpl_propertylist*)>(
                             NULL, NULL);  // TODO: Gotta deal with the unique
                                           // pointer deallocating immediately

  cpl_frameset* usedframes_ptr = usedframes->to_cpl();  // Creates duplicate

  const cpl_frame* inherit_ptr = (inherit != NULL) ? inherit->ptr() : NULL;

  const char* remregexp_ptr =
      (remregexp.has_value()) ? remregexp.value().c_str() : NULL;

  cpl::core::Error::throw_errors_with(
      cpl_dfs_save_image, allframes_ptr, header_ptr.get(), parlist->ptr().get(),
      usedframes_ptr, inherit_ptr, image->ptr(), image->get_type(),
      recipe.c_str(), applist->ptr().get(), remregexp_ptr, pipe_id.c_str(),
      filename.c_str());
  cpl::core::Error::throw_errors_with(cpl_frameset_delete, allframes_ptr);
  cpl::core::Error::throw_errors_with(cpl_frameset_delete, usedframes_ptr);
}

void
save_table(std::shared_ptr<cpl::ui::FrameSet> allframes,
           std::shared_ptr<cpl::ui::ParameterList> parlist,
           std::shared_ptr<cpl::ui::FrameSet> usedframes,
           std::shared_ptr<cpl::core::Table> table, const std::string& recipe,
           std::shared_ptr<cpl::core::PropertyList> applist,
           const std::string& pipe_id, const std::string& filename,
           std::shared_ptr<cpl::core::PropertyList> header,  // Optional
           std::shared_ptr<cpl::core::PropertyList> tablelist,
           std::shared_ptr<cpl::ui::Frame> inherit,
           std::optional<std::string> remregexp)
{
  cpl_frameset* allframes_ptr = allframes->to_cpl();  // Creates duplicate

  std::unique_ptr<struct _cpl_propertylist_, void (*)(cpl_propertylist*)>
      header_ptr = (header != NULL)
                       ? header->ptr()
                       : std::unique_ptr<struct _cpl_propertylist_,
                                         void (*)(cpl_propertylist*)>(
                             NULL, NULL);  // TODO: Gotta deal with the unique
                                           // pointer deallocating immediately

  std::unique_ptr<struct _cpl_propertylist_, void (*)(cpl_propertylist*)>
      tablelist_ptr =
          (tablelist != NULL)
              ? tablelist->ptr()
              : std::unique_ptr<struct _cpl_propertylist_,
                                void (*)(cpl_propertylist*)>(
                    NULL, NULL);  // TODO: Gotta deal with the unique pointer
                                  // deallocating immediately

  cpl_frameset* usedframes_ptr = usedframes->to_cpl();  // Creates duplicate

  const cpl_frame* inherit_ptr = (inherit != NULL) ? inherit->ptr() : NULL;

  const char* remregexp_ptr =
      (remregexp.has_value()) ? remregexp.value().c_str() : NULL;

  cpl::core::Error::throw_errors_with(
      cpl_dfs_save_table, allframes_ptr, header_ptr.get(), parlist->ptr().get(),
      usedframes_ptr, inherit_ptr, table->ptr(), tablelist_ptr.get(),
      recipe.c_str(), applist->ptr().get(), remregexp_ptr, pipe_id.c_str(),
      filename.c_str());
  cpl::core::Error::throw_errors_with(cpl_frameset_delete, allframes_ptr);
  cpl::core::Error::throw_errors_with(cpl_frameset_delete, usedframes_ptr);
}
}  // namespace dfs
}  // namespace cpl