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

#ifndef PYCPL_DFS_HPP
#define PYCPL_DFS_HPP

#include <memory>
#include <optional>
#include <string>

#include <cpl_dfs.h>
#include <cpl_type.h>

#include "cplcore/image.hpp"
#include "cplcore/imagelist.hpp"
#include "cplcore/propertylist.hpp"
#include "cplcore/table.hpp"
#include "cplui/frame.hpp"
#include "cplui/frameset.hpp"
#include "cplui/parameterlist.hpp"

namespace cpl
{
namespace dfs
{
/**
 * @brief Update DFS and DICB required header information of product frames
 * within this frameset
 * @param set The frameset from which the product frames are taken.
 * @param md5 true/false on whether to compute and write the md5 data hash
 * @param checksum true/false on whether to compute and write the standard FITS
 * checksums
 *
 * The function takes all frames marked as products from the input frameset
 *
 * @return none
 */
void
sign_products(std::shared_ptr<cpl::ui::FrameSet> set, bool md5, bool checksum);


/**
 * @brief  Perform any DFS-compliancy required actions (DATAMD5/PIPEFILE update)
 * @param  set     The list of frames with FITS products created by the recipe
 * @return none
 * @note Each product frame must correspond to a FITS file created with a CPL
 *       FITS saving function.
 *
 * @throws DataNotFoundError if the input framelist contains a frame of type
 * product with a missing filename.
 * @throws BadFileFormatError if the input framelist contains a frame of type
 * product without a FITS card with key 'DATAMD5'.
 *
 * @throws BadFileFormatError if the The input framelist contains a frame of
 * type product for which the FITS card with key 'DATAMD5' could not be updated.
 */
void update_product_header(std::shared_ptr<cpl::ui::FrameSet> set);


/**
 * @brief    Add product keywords to a pipeline product property list.
 *
 * @param    header          Property list where keywords must be written
 * @param    product_frame   Frame describing the product
 * @param    framelist       List of frames including all input frames
 * @param    parlist         Recipe parameter list
 * @param    recid           Recipe name
 * @param    pipeline_id     Pipeline unique identifier
 * @param    dictionary_id   PRO dictionary identifier
 * @param    inherit_frame   Frame from which header information is inherited
 *
 * @return   none
 *
 * @throws DataNotFoundError if the input framelist contains no input frames or
 *         a frame in the input framelist does not specify a file.
 *         In the former case the string "Empty set-of-frames" is appended
 *         to the error message.
 * @throws IllegalInputError if the product frame is not tagged or not grouped
 *         as cpl.Frame.FrameGroup.PRODUCT
 *         A specified @em inherit_frame doesn't belong to the input frame
 *         list, or it is not in FITS format.
 * @throws FileNotFoundError if a frame in the input framelist specifies a
 *         non-existing file.
 * @throws BadFileFormatError if a frame in the input framelist specifies an
 *         invalid file.
 *
 *
 * This function checks the @em header associated to a pipeline product,
 * to ensure that it is DICB compliant. In particular, this function does
 * the following:
 *
 *    -# Selects a reference frame from which the primary and secondary
 *       keyword information is inherited. The primary information is
 *       contained in the FITS keywords ORIGIN, TELESCOPE, INSTRUME,
 *       OBJECT, RA, DEC, EPOCH, EQUINOX, RADECSYS, DATE-OBS, MJD-OBS,
 *       UTC, LST, PI-COI, OBSERVER, while the secondary information is
 *       contained in all the other keywords. If the @em inherit_frame
 *       is just a NULL pointer, both primary and secondary information
 *       is inherited from the first frame in the input framelist with
 *       group CPL_FRAME_GROUP_RAW, or if no such frames are present
 *       the first frame with group CPL_FRAME_GROUP_CALIB.
 *       If @em inherit_frame is non-NULL, the secondary information
 *       is inherited from @em inherit_frame instead.
 *
 *    -# Copy to @em header, if they are present, the following primary
 *       FITS keywords from the first input frame in the @em framelist:
 *       ORIGIN, TELESCOPE, INSTRUME, OBJECT, RA, DEC, EPOCH, EQUINOX,
 *       RADESYS, DATE-OBS, MJD-OBS, UTC, LST, PI-COI, OBSERVER. If those
 *       keywords are already present in the @em header property list, they
 *       are overwritten only in case they have the same type. If any of
 *       these keywords are present with an unexpected type, a warning is
 *       issued, but the keywords are copied anyway (provided that the
 *       above conditions are fulfilled), and no error is set.
 *
 *    -# Copy all the HIERARCH.ESO._ keywords from the primary FITS header
 *       of the @em inherit_frame in @em framelist, with the exception of
 *       the HIERARCH.ESO.DPR._, and of the .PRO._ and .DRS._ keywords if
 *       the @em inherit_frame is a calibration. If those keywords are
 *       already present in @em header, they are overwritten.
 *
 *    -# If found, remove the HIERARCH.ESO.DPR._ keywords from @em header.
 *
 *    -# If found, remove the ARCFILE and ORIGFILE keywords from @em header.
 *
 *    -# Add to @em header the following mandatory keywords from the PRO
 *       dictionary: PIPEFILE, PRO.DID, PRO.REC1.ID, PRO.REC1.DRS.ID,
 *       PRO.REC1.PIPE.ID, and PRO.CATG. If those keywords are already
 *       present in @em header, they are overwritten. The keyword
 *       PRO.CATG is always set identical to the tag in @em product_frame.
 *
 *    -# Only if missing, add to @em header the following mandatory keywords
 *       from the PRO dictionary: PRO.TYPE, PRO.TECH, and PRO.SCIENCE.
 *       The keyword PRO.TYPE will be set to "REDUCED". If the keyword
 *       DPR.TECH is found in the header of the first frame, PRO.TECH is
 *       given its value, alternatively if the keyword PRO.TECH is found
 *       it is copied instead, and if all fails the value "UNDEFINED" is
 *       set. Finally, if the keyword DPR.CATG is found in the header of
 *       the first frame and is set to "SCIENCE", the boolean keyword
 *       PRO.SCIENCE will be set to "true", otherwise it will be copied
 *       from an existing PRO.SCIENCE keyword, while it will be set to
 *       "false" in all other cases.
 *
 *    -# Check the existence of the keyword PRO.DATANCOM in @em header. If
 *       this keyword is missing, one is added, with the value of the total
 *       number of raw input frames.
 *
 *    -# Add to @em header the keywords PRO.REC1.RAW1.NAME, PRO.REC1.RAW1.CATG,
 *       PRO.REC1.CAL1.NAME, PRO.REC1.CAL1.CATG, to describe the content of
 *       the input set-of-frames.
 *
 * See the DICB PRO dictionary to have details on the mentioned PRO keywords.
 *
 * @note
 *   Non-FITS files are handled as files with an empty FITS header.
 */
void
setup_product_header(std::shared_ptr<cpl::core::PropertyList> header,
                     std::shared_ptr<cpl::ui::Frame> product_frame,
                     std::shared_ptr<cpl::ui::FrameSet> framelist,
                     std::shared_ptr<cpl::ui::ParameterList> parlist,
                     const std::string& recid, const std::string& pipeline_id,
                     const std::string& dictionary_id,
                     std::shared_ptr<cpl::ui::Frame> inherit_frame);

/**
 * @brief   Create a new PAF file
 * @param   instrume Name of instrument in capitals (NACO, VISIR, etc.)
 * @param   recipe   Name of recipe
 * @param   paflist  Propertylist to save
 * @param   filename Filename of created PArameter File
 * @return  none
 */
void save_paf(const std::string& instrume, const std::string& recipe,
              std::shared_ptr<cpl::core::PropertyList> paflist,
              const std::string& filename);

/**
* @brief  Save a propertylist as a DFS-compliant pipeline product
* @param  allframes  The list of input frames for the recipe
* @param  header     NULL, or filled with properties written to product header
* @param  parlist    The list of input parameters
* @param  usedframes The list of raw/calibration frames used for this product
* @param  inherit    NULL or product frames inherit their header from this frame
* @param  recipe     The recipe name
* @param  applist    Propertylist to append to primary header, w. PRO.CATG
* @param  remregexp  Optional regexp of properties not to put in main header
* @param  pipe_id    PACKAGE "/" PACKAGE_VERSION
* @param  filename   Filename of created product
* @note remregexp may be NULL
* @return none
* @see save_image()

The FITS header of the created product is created from the provided applist
and the cards copied by setup_product_header(), with exception of
the cards whose keys match the provided remregexp.

The FITS data unit will be empty.

*/
void save_propertylist(std::shared_ptr<cpl::ui::FrameSet> allframes,
                       std::shared_ptr<cpl::ui::ParameterList> parlist,
                       std::shared_ptr<cpl::ui::FrameSet> usedframes,
                       const std::string& recipe,
                       std::shared_ptr<cpl::core::PropertyList> applist,
                       const std::string& pipe_id, const std::string& filename,
                       std::shared_ptr<cpl::core::PropertyList> header,
                       std::shared_ptr<cpl::ui::Frame> inherit,
                       std::optional<std::string> remregexp);
/**
* @brief  Save an imagelist as a DFS-compliant pipeline product
* @param  allframes  The list of input frames for the recipe
* @param  header     NULL, or filled with properties written to product header
* @param  parlist    The list of input parameters
* @param  usedframes The list of raw/calibration frames used for this product
* @param  inherit    NULL or product frames inherit their header from this frame
* @param  imagelist  The imagelist to be saved
* @param  type       The type used to represent the data in the file
* @param  recipe     The recipe name
* @param  applist    Propertylist to append to primary header, w. PRO.CATG
* @param  remregexp  Optional regexp of properties not to put in main header
* @param  pipe_id    PACKAGE "/" PACKAGE_VERSION
* @param  filename   Filename of created product
* @note remregexp may be NULL
* @return none
* @see cpl::dfs::save_image(), cpl::dfs::save_propertylist()
The FITS header of the created product is created from the provided applist
and the cards copied by cpl_dfs_setup_product_header(), with exception of
the cards whose keys match the provided remregexp.
*/
void save_imagelist(std::shared_ptr<cpl::ui::FrameSet> allframes,
                    std::shared_ptr<cpl::ui::ParameterList> parlist,
                    std::shared_ptr<cpl::ui::FrameSet> usedframes,
                    std::shared_ptr<cpl::core::ImageList> imagelist,
                    cpl_type type, const std::string& recipe,
                    std::shared_ptr<cpl::core::PropertyList> applist,
                    const std::string& pipe_id, const std::string& filename,
                    std::shared_ptr<cpl::core::PropertyList> header,
                    std::shared_ptr<cpl::ui::Frame> inherit,
                    std::optional<std::string> remregexp);

/**
 @brief  Save an image as a DFS-compliant pipeline product
* @param  allframes  The list of input frames for the recipe
* @param  header     NULL, or filled with properties written to product header
* @param  parlist    The list of input parameters
* @param  usedframes The list of raw/calibration frames used for this product
* @param  inherit    NULL or product frames inherit their header from this frame
* @param  image      The image to be saved
* @param  type       The type used to represent the data in the file
* @param  recipe     The recipe name
* @param  applist    Propertylist to append to primary header, w. PRO.CATG
* @param  remregexp  Optional regexp of properties not to put in main header
* @param  pipe_id    PACKAGE "/" PACKAGE_VERSION
* @param  filename   Filename of created product
* @note The image may be NULL in which case only the header information is saved
     but passing a NULL image is deprecated, use cpl_dfs_save_propertylist().
* @note remregexp may be NULL
* @note applist must contain a string-property with key CPL_DFS_PRO_CATG
* @note On success and iff header is non-NULL, it will be emptied and then
     filled with the properties written to the primary header of the product
* @see cpl::dfs::setup_product_header(), cpl::dfs::image_save().

The FITS header of the created product is created from the provided applist
and the cards copied by setup_product_header(), with exception of
the cards whose keys match the provided remregexp.

*/
void save_image(std::shared_ptr<cpl::ui::FrameSet> allframes,
                std::shared_ptr<cpl::ui::ParameterList> parlist,
                std::shared_ptr<cpl::ui::FrameSet> usedframes,
                std::shared_ptr<cpl::core::ImageBase> image,
                const std::string& recipe,
                std::shared_ptr<cpl::core::PropertyList> applist,
                const std::string& pipe_id, const std::string& filename,
                std::shared_ptr<cpl::core::PropertyList> header,
                std::shared_ptr<cpl::ui::Frame> inherit,
                std::optional<std::string> remregexp);


/**
@brief  Save a table as a DFS-compliant pipeline product
* @param  allframes  The list of input frames for the recipe
* @param  header     NULL, or filled with properties written to product header
* @param  parlist    The list of input parameters
* @param  usedframes The list of raw/calibration frames used for this product
* @param  inherit    NULL or product frames inherit their header from this frame
* @param  table      The table to be saved
* @param  tablelist  Optional propertylist to use in table extension or NULL
* @param  recipe     The recipe name
* @param  applist    Propertylist to append to primary header, w. PRO.CATG
* @param  remregexp  Optional regexp of properties not to put in main header
* @param  pipe_id    PACKAGE "/" PACKAGE_VERSION
* @param  filename   Filename of created product
* @see cpl::dfs::save_image()
The FITS header of the created product is created from the provided applist
and the cards copied by setup_product_header(), with exception of
the cards whose keys match the provided remregexp.
*/
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
           std::optional<std::string> remregexp);

}  // namespace dfs
}  // namespace cpl

#endif  // PYCPL_DFS_HPP