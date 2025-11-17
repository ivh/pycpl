/*
 * This file is part of the ESO Common Pipeline Library
 * Copyright (C) 2001-2022 European Southern Observatory
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "cpl_multiframe.h"
#include "cpl_test.h"

#include <fitsio.h>
#include <string.h>

// Test image data size
static const cpl_size DATA_SIZE = 64;

// Test values of different types
static const cpl_boolean BOOL_VALUES[3] = { CPL_TRUE, CPL_FALSE, CPL_TRUE };
static const int INT_VALUES[3] = { 0, 1, 2 };
static const float FLOAT_VALUES[3] = { 100., 200., -300. };
static const float _Complex CMPLX_VALUES[3] = { 1. + 2 * _Complex_I,
                                                3. + -4. * _Complex_I,
                                                -5. + -6. * _Complex_I };

static const char CHAR_VALUES[3] = { 'a', 'b', 'c' };
static const char *const STRING_VALUES[4] = {
    "Toulouse", "Berlioz", "Thomas O'Malley",
    "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789"
};

// File names and frame tags
static const char *const INPUT_FILES[2] = { "multiframe1.fits",
                                            "multiframe2.fits" };
static const char *const OUTPUT_FILE = "multiframe.fits";
static const char *const FRAME_TAGS[2] = { "TEST1", "TEST2" };

// Data set labels (unique identifiers)
static const char *const LABELS[3] = { "ABC123@_&!*", "DATA", "STATS" };

// Scratchpad
#define BUFFER_SIZE 64
static char TEST_BUFFER[BUFFER_SIZE] = { '\0' };

// Internal utilities
static cpl_propertylist *
_cpl_test_multiframe_generate_header_primary(void)
{
    cpl_propertylist *properties = cpl_propertylist_new();
    cpl_propertylist_update_string(properties, "EXTNAME", "PRIMARY_HDU");
    cpl_propertylist_update_int(properties, "EXTVER", 1);
    cpl_propertylist_update_int(properties, "EXTLEVEL", 1);
    cpl_propertylist_update_string(properties, "INSTRUME", "INSTRUMENT");
    cpl_propertylist_set_comment(properties, "INSTRUME", "Instrument used");
    cpl_propertylist_update_float(properties, "EXPTIME", 1.0);
    cpl_propertylist_set_comment(properties, "EXPTIME", "Integration time");
    cpl_propertylist_update_float(properties, "MJD-OBS", 58150.34768681);
    cpl_propertylist_set_comment(properties, "MJD-OBS", "Observation start");
    cpl_propertylist_update_string(properties, "DATE-OBS",
                                   "2018-02-01T08:20:40.000");
    cpl_propertylist_set_comment(properties, "DATE-OBS", "Observing date");

    cpl_propertylist_update_float(properties, "BSCALE", 1.0);
    cpl_propertylist_update_float(properties, "BZERO", 32768.0);
    cpl_propertylist_update_string(properties, "BUNIT", "counts");
    cpl_propertylist_update_float(properties, "DATAMIN", 0.);
    cpl_propertylist_update_float(properties, "DATAMAX", 1.);
    cpl_propertylist_update_int(properties, "BLANK", -1);

    for (size_t idx = 0; idx < 3; ++idx) {
        char key[FLEN_KEYWORD];
        snprintf(key, FLEN_KEYWORD, "ESO QC LOGICAL VALUE%zd", idx);
        cpl_propertylist_append_bool(properties, key, BOOL_VALUES[idx]);
    }

    for (size_t idx = 0; idx < 3; ++idx) {
        char key[FLEN_KEYWORD];
        snprintf(key, FLEN_KEYWORD, "ESO QC INTEGER VALUE%zd", idx);
        cpl_propertylist_append_int(properties, key, INT_VALUES[idx]);
    }

    for (size_t idx = 0; idx < 3; ++idx) {
        char key[FLEN_KEYWORD];
        snprintf(key, FLEN_KEYWORD, "ESO QC FLOAT VALUE%zd", idx);
        cpl_propertylist_append_double(properties, key, FLOAT_VALUES[idx]);
    }

    for (size_t idx = 0; idx < 3; ++idx) {
        char key[FLEN_KEYWORD];
        snprintf(key, FLEN_KEYWORD, "ESO QC COMPLEX VALUE%zd", idx);
        cpl_propertylist_append_double_complex(properties, key,
                                               CMPLX_VALUES[idx]);
    }

    for (size_t idx = 0; idx < 3; ++idx) {
        char key[FLEN_KEYWORD];
        snprintf(key, FLEN_KEYWORD, "ESO QC CHAR VALUE%zd", idx);
        cpl_propertylist_append_char(properties, key, CHAR_VALUES[idx]);
    }

    for (size_t idx = 0; idx < 3; ++idx) {
        char key[FLEN_KEYWORD];
        snprintf(key, FLEN_KEYWORD, "ESO QC STRING VALUE%zd", idx);
        cpl_propertylist_append_string(properties, key, STRING_VALUES[idx]);
    }

    return properties;
}

static cpl_propertylist *
_cpl_test_multiframe_generate_header_extension(const char *extname)
{
    cpl_propertylist *properties = cpl_propertylist_new();
    if (extname) {
        cpl_propertylist_update_string(properties, "EXTNAME", extname);
    }
    cpl_propertylist_update_bool(properties, "INHERIT", CPL_FALSE);
    cpl_propertylist_update_float(properties, "DATAMIN", -1.);
    cpl_propertylist_update_float(properties, "DATAMAX", 0.);
    cpl_propertylist_update_string(properties, "BUNIT", "adu");
    cpl_propertylist_update_bool(properties, "ESO DRS LOGICAL VALUE",
                                 CPL_FALSE);
    cpl_propertylist_set_comment(properties, "ESO DRS LOGICAL VALUE",
                                 "A boolean value");
    cpl_propertylist_update_int(properties, "ESO DRS INT VALUE", INT_VALUES[2]);
    cpl_propertylist_set_comment(properties, "ESO DRS INT VALUE",
                                 "An integer value");
    cpl_propertylist_update_float(properties, "ESO DRS FLOAT VALUE",
                                  FLOAT_VALUES[1]);
    cpl_propertylist_set_comment(properties, "ESO DRS FLOAT VALUE",
                                 "A float value");
    cpl_propertylist_update_double(properties, "ESO DRS DOUBLE VALUE",
                                   FLOAT_VALUES[1] * 1e-24);
    cpl_propertylist_set_comment(properties, "ESO DRS DOUBLE VALUE",
                                 "A double value");
    cpl_propertylist_update_double_complex(properties, "ESO DRS COMPLEX VALUE",
                                           CMPLX_VALUES[2]);
    cpl_propertylist_set_comment(properties, "ESO DRS COMPLEX VALUE",
                                 "A complex value");
    cpl_propertylist_update_char(properties, "ESO DRS CHAR VALUE",
                                 CHAR_VALUES[2]);
    cpl_propertylist_set_comment(properties, "ESO DRS CHAR VALUE",
                                 "A character value");
    cpl_propertylist_update_string(properties, "ESO DRS STRING VALUE",
                                   STRING_VALUES[2]);
    cpl_propertylist_set_comment(properties, "ESO DRS STRING VALUE",
                                 "A string value");
    return properties;
}

// Generate a simple dataset, an image in the primary HDU with a header.
static cpl_error_code
_cpl_test_multiframe_generate_dataset1(const char *filename,
                                       cpl_propertylist *properties)
{
    cpl_image *image = cpl_image_new(DATA_SIZE, DATA_SIZE, CPL_TYPE_INT);
    cpl_test_nonnull(image);
    cpl_image_fill_window(image, 1, 1, DATA_SIZE, DATA_SIZE, 1.);
    cpl_error_code ecode = cpl_image_save(image, filename, CPL_TYPE_INT,
                                          properties, CPL_IO_CREATE);
    cpl_test_eq_error(ecode, CPL_ERROR_NONE);
    cpl_image_delete(image);
    return CPL_ERROR_NONE;
}

// Generate a dataset that has an empty primary HDU (header-only) followed by
// two extensions, a cube and an image. The extensions headers can be specified
// through the properties argument. The extension headers may be NULL.
static cpl_error_code
_cpl_test_multiframe_generate_dataset2(const char *filename,
                                       const cpl_propertylist *properties[3])
{
    cpl_test_assert(properties[0] != NULL);

    cpl_image *image = cpl_image_new(DATA_SIZE, DATA_SIZE, CPL_TYPE_FLOAT);
    cpl_test_nonnull(image);
    cpl_image_fill_window(image, 1, 1, DATA_SIZE, DATA_SIZE, 0.5);

    cpl_imagelist *cube = cpl_imagelist_new();
    cpl_test_nonnull(cube);
    for (size_t i = 0; i < 3; ++i) {
        cpl_imagelist_set(cube, cpl_image_duplicate(image), i);
    }

    // Assemble the test dataset
    cpl_propertylist_save(properties[0], filename, CPL_IO_CREATE);
    cpl_imagelist_save(cube, filename, CPL_TYPE_FLOAT, properties[1],
                       CPL_IO_EXTEND);
    cpl_image_save(image, filename, CPL_TYPE_INT, properties[2], CPL_IO_EXTEND);
    cpl_imagelist_delete(cube);
    cpl_image_delete(image);
    return CPL_ERROR_NONE;
}


int
main(void)
{
    cpl_test_init(PACKAGE_BUGREPORT, CPL_MSG_WARNING);

    cpl_multiframe *multiframe = NULL;
    cpl_frame *frame1 = NULL;
    cpl_frame *frame2 = NULL;
    cpl_propertylist *hdr_product = NULL;
    cpl_propertylist *hdr_ext = NULL;
    cpl_error_code ecode = CPL_ERROR_NONE;

    // Create the initial file and the associated frame object from which
    // the multiframe container is constructed.
    cpl_propertylist *hdr_main = _cpl_test_multiframe_generate_header_primary();
    cpl_propertylist_save(hdr_main, INPUT_FILES[0], CPL_IO_CREATE);

    frame1 = cpl_frame_new();
    cpl_frame_set_filename(frame1, INPUT_FILES[0]);
    cpl_frame_set_tag(frame1, FRAME_TAGS[0]);


    // Test 1: Attempt creating a multiframe container with a NULL id
    multiframe = cpl_multiframe_new(frame1, NULL, NULL);
    cpl_test_null(multiframe);
    cpl_test_error(CPL_ERROR_NULL_INPUT);


    // Test 2: Create an new, empty multiframe container, from a header only
    // original dataset. The multiframe container is created without and id, and
    // no filter is applied.
    multiframe = cpl_multiframe_new(frame1, "", NULL);
    cpl_test_nonnull(multiframe);
    cpl_test_eq(cpl_multiframe_get_size(multiframe), 1);
    cpl_test_eq_string(cpl_multiframe_dataset_get_id(multiframe, 0), "");

    // Test 2a: Save the multiframe container to a file.
    ecode = cpl_multiframe_write(multiframe, OUTPUT_FILE);
    cpl_test_eq_error(ecode, CPL_ERROR_NONE);

    // Test 2b: Verify that the saved multiframe container does not contain the
    // keywords BSCALE, BZERO, BUNIT, BLANK, DATAMIN, DATAMAX, and the keywords
    // defined by the _CPL_FITSDATASET_IGNORE_KEYPATTERN pattern.

    // Load the header of the saved multiframe again, check that the keywords
    // have been removed.
    hdr_product = cpl_propertylist_load(OUTPUT_FILE, 0);
    cpl_test_nonnull(hdr_product);
    cpl_test_zero(cpl_propertylist_has(hdr_product, "EXTNAME"));
    cpl_test_zero(cpl_propertylist_has(hdr_product, "EXTVER"));
    cpl_test_zero(cpl_propertylist_has(hdr_product, "EXTLEVEL"));
    cpl_test_zero(cpl_propertylist_has(hdr_product, "BSCALE"));
    cpl_test_zero(cpl_propertylist_has(hdr_product, "BZERO"));
    cpl_test_zero(cpl_propertylist_has(hdr_product, "BUNIT"));
    cpl_test_zero(cpl_propertylist_has(hdr_product, "BLANK"));
    cpl_test_zero(cpl_propertylist_has(hdr_product, "DATAMIN"));
    cpl_test_zero(cpl_propertylist_has(hdr_product, "DATAMAX"));
    cpl_propertylist_delete(hdr_product);

    cpl_multiframe_delete(multiframe);
    multiframe = NULL;


    // Test 3: Create a multiframe container with an id. Verify that the id is
    // can be retrieved and is set correctly.
    multiframe = cpl_multiframe_new(frame1, LABELS[0], NULL);
    cpl_test_nonnull(multiframe);
    cpl_test_eq_string(cpl_multiframe_dataset_get_id(multiframe, 0), LABELS[0]);
    cpl_test_eq(cpl_multiframe_dataset_get_position(multiframe, LABELS[0]), 0);

    ecode = cpl_multiframe_write(multiframe, OUTPUT_FILE);
    cpl_test_eq_error(ecode, CPL_ERROR_NONE);

    hdr_product = cpl_propertylist_load(OUTPUT_FILE, 0);
    cpl_test_nonnull(hdr_product);
    cpl_test_eq(cpl_propertylist_has(hdr_product, "EXTNAME"), 1);
    cpl_test_eq_string(cpl_propertylist_get_string(hdr_product, "EXTNAME"),
                       LABELS[0]);
    cpl_propertylist_delete(hdr_product);

    cpl_multiframe_delete(multiframe);
    multiframe = NULL;

    // Test 4: Create a multiframe container with a custom filter. Verify that
    // the filter was applied successfully

    cpl_regex *filter = cpl_regex_new("ESO QC (CHAR|FLOAT).*", CPL_TRUE,
                                      CPL_REGEX_EXTENDED | CPL_REGEX_NOSUBS);
    multiframe = cpl_multiframe_new(frame1, LABELS[0], filter);
    cpl_test_nonnull(multiframe);

    ecode = cpl_multiframe_write(multiframe, OUTPUT_FILE);
    cpl_test_eq_error(ecode, CPL_ERROR_NONE);

    hdr_product = cpl_propertylist_load(OUTPUT_FILE, 0);
    cpl_test_nonnull(hdr_product);
    cpl_test_zero(cpl_propertylist_has(hdr_product, "ESO QC CHAR VALUE0"));
    cpl_test_zero(cpl_propertylist_has(hdr_product, "ESO QC CHAR VALUE1"));
    cpl_test_zero(cpl_propertylist_has(hdr_product, "ESO QC CHAR VALUE2"));
    cpl_test_zero(cpl_propertylist_has(hdr_product, "ESO QC FLOAT VALUE0"));
    cpl_test_zero(cpl_propertylist_has(hdr_product, "ESO QC FLOAT VALUE1"));
    cpl_test_zero(cpl_propertylist_has(hdr_product, "ESO QC FLOAT VALUE2"));
    cpl_test_eq(cpl_propertylist_has(hdr_product, "ESO QC INTEGER VALUE0"), 1);
    cpl_test_eq(cpl_propertylist_has(hdr_product, "ESO QC INTEGER VALUE1"), 1);
    cpl_test_eq(cpl_propertylist_has(hdr_product, "ESO QC INTEGER VALUE2"), 1);
    cpl_propertylist_delete(hdr_product);

    cpl_multiframe_delete(multiframe);
    cpl_regex_delete(filter);
    filter = NULL;
    multiframe = NULL;

    // Test 5: Create a new, empty multiframe container from an original dataset
    // that contains data in the primary data unit.
    _cpl_test_multiframe_generate_dataset1(INPUT_FILES[0], hdr_main);

    multiframe = cpl_multiframe_new(frame1, LABELS[0], NULL);
    cpl_test_nonnull(multiframe);

    ecode = cpl_multiframe_write(multiframe, OUTPUT_FILE);
    cpl_test_eq_error(ecode, CPL_ERROR_NONE);

    cpl_multiframe_delete(multiframe);
    multiframe = NULL;

    // Test 6: Create a new multiframe containter is created and append
    // datasets to it.
    const cpl_propertylist *properties[3] = { hdr_main, NULL, NULL };
    _cpl_test_multiframe_generate_dataset2(INPUT_FILES[0], properties);
    _cpl_test_multiframe_generate_dataset2(INPUT_FILES[1], properties);

    // Create a multiframe container from the input files
    multiframe = cpl_multiframe_new(frame1, LABELS[0], NULL);
    cpl_test_nonnull(multiframe);

    // Add dataset without applying any filter
    ecode = cpl_multiframe_append_dataset_from_position(multiframe, LABELS[1],
                                                        frame1, 1, NULL, NULL,
                                                        CPL_MULTIFRAME_ID_SET);
    cpl_test_eq_error(ecode, CPL_ERROR_NONE);

    ecode = cpl_multiframe_write(multiframe, OUTPUT_FILE);
    cpl_test_eq_error(ecode, CPL_ERROR_NONE);

    // Check keyword contents of the added dataset. The header contents
    // should have all the keywords from the hdr_primary, except for the
    // keywords which should not appear in an extension header, or are
    // specific to the extension (e.g. EXTNAME, EXTVER, EXTLEVEL).
    hdr_product = cpl_propertylist_load(OUTPUT_FILE, 1);
    cpl_test_nonnull(hdr_product);
    cpl_test_eq(cpl_propertylist_has(hdr_product, "INSTRUME"), 1);
    cpl_test_eq(cpl_propertylist_has(hdr_product, "EXPTIME"), 1);
    cpl_test_eq(cpl_propertylist_has(hdr_product, "MJD-OBS"), 1);
    cpl_test_eq(cpl_propertylist_has(hdr_product, "DATE-OBS"), 1);
    cpl_test_eq(cpl_propertylist_has(hdr_product, "BUNIT"), 1);
    cpl_test_eq(cpl_propertylist_has(hdr_product, "DATAMIN"), 1);
    cpl_test_eq(cpl_propertylist_has(hdr_product, "DATAMAX"), 1);
    cpl_test_eq(cpl_propertylist_has(hdr_product, "BLANK"), 1);
    cpl_test_eq(cpl_propertylist_has(hdr_product, "ESO QC LOGICAL VALUE0"), 1);
    cpl_test_eq(cpl_propertylist_has(hdr_product, "ESO QC LOGICAL VALUE1"), 1);
    cpl_test_eq(cpl_propertylist_has(hdr_product, "ESO QC LOGICAL VALUE2"), 1);
    cpl_test_eq(cpl_propertylist_has(hdr_product, "ESO QC INTEGER VALUE0"), 1);
    cpl_test_eq(cpl_propertylist_has(hdr_product, "ESO QC INTEGER VALUE1"), 1);
    cpl_test_eq(cpl_propertylist_has(hdr_product, "ESO QC INTEGER VALUE2"), 1);
    cpl_test_eq(cpl_propertylist_has(hdr_product, "ESO QC FLOAT VALUE0"), 1);
    cpl_test_eq(cpl_propertylist_has(hdr_product, "ESO QC FLOAT VALUE1"), 1);
    cpl_test_eq(cpl_propertylist_has(hdr_product, "ESO QC FLOAT VALUE2"), 1);
    cpl_test_eq(cpl_propertylist_has(hdr_product, "ESO QC COMPLEX VALUE0"), 1);
    cpl_test_eq(cpl_propertylist_has(hdr_product, "ESO QC COMPLEX VALUE1"), 1);
    cpl_test_eq(cpl_propertylist_has(hdr_product, "ESO QC COMPLEX VALUE2"), 1);
    cpl_test_eq(cpl_propertylist_has(hdr_product, "ESO QC CHAR VALUE0"), 1);
    cpl_test_eq(cpl_propertylist_has(hdr_product, "ESO QC CHAR VALUE1"), 1);
    cpl_test_eq(cpl_propertylist_has(hdr_product, "ESO QC CHAR VALUE2"), 1);
    cpl_test_eq(cpl_propertylist_has(hdr_product, "ESO QC STRING VALUE0"), 1);
    cpl_test_eq(cpl_propertylist_has(hdr_product, "ESO QC STRING VALUE1"), 1);
    cpl_test_eq(cpl_propertylist_has(hdr_product, "ESO QC STRING VALUE2"), 1);

    // Check values to validate formatting.
    cpl_test_eq(strcmp(cpl_propertylist_get_string(hdr_product, "EXTNAME"),
                       LABELS[1]),
                0);
    cpl_test_eq(cpl_propertylist_get_bool(hdr_product, "ESO QC LOGICAL VALUE0"),
                BOOL_VALUES[0]);
    cpl_test_eq(cpl_propertylist_get_int(hdr_product, "ESO QC INTEGER VALUE0"),
                INT_VALUES[0]);
    cpl_test_eq(cpl_propertylist_get_float(hdr_product, "ESO QC FLOAT VALUE0"),
                FLOAT_VALUES[0]);
    cpl_test_eq(cpl_propertylist_get_double(hdr_product, "ESO QC FLOAT VALUE0"),
                FLOAT_VALUES[0]);
    cpl_test_eq(cpl_propertylist_get_float_complex(hdr_product,
                                                   "ESO QC COMPLEX VALUE0"),
                CMPLX_VALUES[0]);
    cpl_test_eq(strcmp(cpl_propertylist_get_string(hdr_product,
                                                   "ESO QC CHAR VALUE0"),
                       "a"),
                0);
    cpl_test_eq(strcmp(cpl_propertylist_get_string(hdr_product,
                                                   "ESO QC STRING VALUE0"),
                       STRING_VALUES[0]),
                0);
    cpl_propertylist_delete(hdr_product);

    cpl_multiframe_delete(multiframe);
    multiframe = NULL;

    // Test 7: Create a multiframe container and add a dataset to it applying
    // a filter to the primary header of the source dataset.
    multiframe = cpl_multiframe_new(frame1, LABELS[0], NULL);
    cpl_test_nonnull(multiframe);

    // Create a filter and apply it to the primary HDU header of the dataset
    cpl_regex *ignore_keys_main =
        cpl_regex_new("(^DATAMIN|^DATAMAX|ESO QC.*)", CPL_TRUE,
                      CPL_REGEX_EXTENDED | CPL_REGEX_NOSUBS);

    ecode = cpl_multiframe_append_dataset_from_position(multiframe, LABELS[1],
                                                        frame1, 1,
                                                        ignore_keys_main, NULL,
                                                        CPL_MULTIFRAME_ID_SET);
    cpl_test_eq_error(ecode, CPL_ERROR_NONE);
    cpl_regex_delete(ignore_keys_main);
    ignore_keys_main = NULL;

    ecode = cpl_multiframe_write(multiframe, OUTPUT_FILE);
    cpl_test_eq_error(ecode, CPL_ERROR_NONE);

    // Check keyword contents of the added dataset. Verify that
    // all ignored keywords have not been compied to the new
    // dataset header.
    hdr_product = cpl_propertylist_load(OUTPUT_FILE, 1);
    cpl_test_nonnull(hdr_product);
    cpl_test_eq(cpl_propertylist_has(hdr_product, "INSTRUME"), 1);
    cpl_test_eq(cpl_propertylist_has(hdr_product, "EXPTIME"), 1);
    cpl_test_eq(cpl_propertylist_has(hdr_product, "MJD-OBS"), 1);
    cpl_test_eq(cpl_propertylist_has(hdr_product, "DATE-OBS"), 1);
    cpl_test_eq(cpl_propertylist_has(hdr_product, "BUNIT"), 1);
    cpl_test_eq(cpl_propertylist_has(hdr_product, "BLANK"), 1);
    cpl_test_zero(cpl_propertylist_has(hdr_product, "DATAMIN"));
    cpl_test_zero(cpl_propertylist_has(hdr_product, "DATAMAX"));
    cpl_test_zero(cpl_propertylist_has(hdr_product, "ESO QC LOGICAL VALUE0"));
    cpl_test_zero(cpl_propertylist_has(hdr_product, "ESO QC LOGICAL VALUE1"));
    cpl_test_zero(cpl_propertylist_has(hdr_product, "ESO QC LOGICAL VALUE2"));
    cpl_test_zero(cpl_propertylist_has(hdr_product, "ESO QC INTEGER VALUE0"));
    cpl_test_zero(cpl_propertylist_has(hdr_product, "ESO QC INTEGER VALUE1"));
    cpl_test_zero(cpl_propertylist_has(hdr_product, "ESO QC INTEGER VALUE2"));
    cpl_test_zero(cpl_propertylist_has(hdr_product, "ESO QC FLOAT VALUE0"));
    cpl_test_zero(cpl_propertylist_has(hdr_product, "ESO QC FLOAT VALUE1"));
    cpl_test_zero(cpl_propertylist_has(hdr_product, "ESO QC FLOAT VALUE2"));
    cpl_test_zero(cpl_propertylist_has(hdr_product, "ESO QC COMPLEX VALUE0"));
    cpl_test_zero(cpl_propertylist_has(hdr_product, "ESO QC COMPLEX VALUE1"));
    cpl_test_zero(cpl_propertylist_has(hdr_product, "ESO QC COMPLEX VALUE2"));
    cpl_test_zero(cpl_propertylist_has(hdr_product, "ESO QC CHAR VALUE0"));
    cpl_test_zero(cpl_propertylist_has(hdr_product, "ESO QC CHAR VALUE1"));
    cpl_test_zero(cpl_propertylist_has(hdr_product, "ESO QC CHAR VALUE2"));
    cpl_test_zero(cpl_propertylist_has(hdr_product, "ESO QC STRING VALUE0"));
    cpl_test_zero(cpl_propertylist_has(hdr_product, "ESO QC STRING VALUE1"));
    cpl_test_zero(cpl_propertylist_has(hdr_product, "ESO QC STRING VALUE2"));
    cpl_propertylist_delete(hdr_product);

    cpl_multiframe_delete(multiframe);
    multiframe = NULL;

    // Test 8: Create a multiframe container and add a dataset to it applying
    // a filter to both, the primary header and the extension header of
    // the source dataset.
    hdr_ext = _cpl_test_multiframe_generate_header_extension("TEST");
    properties[1] = hdr_ext;
    _cpl_test_multiframe_generate_dataset2(INPUT_FILES[0], properties);
    _cpl_test_multiframe_generate_dataset2(INPUT_FILES[1], properties);

    // Create a multiframe container from the input files
    multiframe = cpl_multiframe_new(frame1, LABELS[0], NULL);
    cpl_test_nonnull(multiframe);

    // Create filters to apply it to the primary HDU header and the extension
    // HDU header of the dataset.
    ignore_keys_main = cpl_regex_new("(^DATAMIN|^DATAMAX|ESO QC.*)", CPL_TRUE,
                                     CPL_REGEX_EXTENDED | CPL_REGEX_NOSUBS);
    cpl_regex *ignore_keys_ext =
        cpl_regex_new("(^BUNIT|^INHERIT|ESO DRS (DOUBLE|CHAR|STRING).*)",
                      CPL_TRUE, CPL_REGEX_EXTENDED | CPL_REGEX_NOSUBS);

    ecode =
        cpl_multiframe_append_dataset_from_position(multiframe, LABELS[1],
                                                    frame1, 1, ignore_keys_main,
                                                    ignore_keys_ext,
                                                    CPL_MULTIFRAME_ID_PREFIX);
    cpl_test_eq_error(ecode, CPL_ERROR_NONE);
    cpl_regex_delete(ignore_keys_ext);
    ignore_keys_ext = NULL;
    cpl_regex_delete(ignore_keys_main);
    ignore_keys_main = NULL;

    ecode = cpl_multiframe_write(multiframe, OUTPUT_FILE);
    cpl_test_eq_error(ecode, CPL_ERROR_NONE);

    // Check keyword contents of the added dataset. Verify that
    // all ignored keywords have not been compied to the new
    // dataset header.
    hdr_product = cpl_propertylist_load(OUTPUT_FILE, 1);
    cpl_test_nonnull(hdr_product);
    cpl_test_eq(cpl_propertylist_has(hdr_product, "INSTRUME"), 1);
    cpl_test_eq(cpl_propertylist_has(hdr_product, "EXPTIME"), 1);
    cpl_test_eq(cpl_propertylist_has(hdr_product, "MJD-OBS"), 1);
    cpl_test_eq(cpl_propertylist_has(hdr_product, "DATE-OBS"), 1);
    cpl_test_eq(cpl_propertylist_has(hdr_product, "BUNIT"), 1);
    cpl_test_eq(cpl_propertylist_has(hdr_product, "BLANK"), 1);
    cpl_test_eq(cpl_propertylist_has(hdr_product, "DATAMIN"), 1);
    cpl_test_eq(cpl_propertylist_has(hdr_product, "DATAMAX"), 1);
    cpl_test_zero(cpl_propertylist_has(hdr_product, "INHERIT"));
    cpl_test_zero(cpl_propertylist_has(hdr_product, "ESO QC LOGICAL VALUE0"));
    cpl_test_zero(cpl_propertylist_has(hdr_product, "ESO QC LOGICAL VALUE1"));
    cpl_test_zero(cpl_propertylist_has(hdr_product, "ESO QC LOGICAL VALUE2"));
    cpl_test_zero(cpl_propertylist_has(hdr_product, "ESO QC INTEGER VALUE0"));
    cpl_test_zero(cpl_propertylist_has(hdr_product, "ESO QC INTEGER VALUE1"));
    cpl_test_zero(cpl_propertylist_has(hdr_product, "ESO QC INTEGER VALUE2"));
    cpl_test_zero(cpl_propertylist_has(hdr_product, "ESO QC FLOAT VALUE0"));
    cpl_test_zero(cpl_propertylist_has(hdr_product, "ESO QC FLOAT VALUE1"));
    cpl_test_zero(cpl_propertylist_has(hdr_product, "ESO QC FLOAT VALUE2"));
    cpl_test_zero(cpl_propertylist_has(hdr_product, "ESO QC COMPLEX VALUE0"));
    cpl_test_zero(cpl_propertylist_has(hdr_product, "ESO QC COMPLEX VALUE1"));
    cpl_test_zero(cpl_propertylist_has(hdr_product, "ESO QC COMPLEX VALUE2"));
    cpl_test_zero(cpl_propertylist_has(hdr_product, "ESO QC CHAR VALUE0"));
    cpl_test_zero(cpl_propertylist_has(hdr_product, "ESO QC CHAR VALUE1"));
    cpl_test_zero(cpl_propertylist_has(hdr_product, "ESO QC CHAR VALUE2"));
    cpl_test_zero(cpl_propertylist_has(hdr_product, "ESO QC STRING VALUE0"));
    cpl_test_zero(cpl_propertylist_has(hdr_product, "ESO QC STRING VALUE1"));
    cpl_test_zero(cpl_propertylist_has(hdr_product, "ESO QC STRING VALUE2"));
    cpl_test_zero(cpl_propertylist_has(hdr_product, "ESO DRS DOUBLE VALUE"));
    cpl_test_zero(cpl_propertylist_has(hdr_product, "ESO DRS CHAR VALUE"));
    cpl_test_zero(cpl_propertylist_has(hdr_product, "ESO DRS STRING VALUE"));

    snprintf(TEST_BUFFER, BUFFER_SIZE, "%s%s", LABELS[1], "TEST");
    cpl_test_eq(strncmp(cpl_propertylist_get_string(hdr_product, "EXTNAME"),
                        TEST_BUFFER, BUFFER_SIZE),
                0);
    cpl_test_eq(strncmp(cpl_multiframe_dataset_get_id(multiframe, 1),
                        TEST_BUFFER, BUFFER_SIZE),
                0);
    TEST_BUFFER[0] = '\0';
    cpl_test_eq(strncmp(cpl_propertylist_get_string(hdr_product, "BUNIT"),
                        "counts", 6),
                0);
    cpl_test_eq(cpl_propertylist_get_float(hdr_product, "DATAMIN"), -1.);
    cpl_test_eq(cpl_propertylist_get_float(hdr_product, "DATAMAX"), 0.);
    cpl_propertylist_delete(hdr_product);

    cpl_multiframe_delete(multiframe);
    multiframe = NULL;

    // Test 9: Same as test 8, but use a different file as dataset source,
    // lookup the source dataset by name, and use a different dataset id
    // generation method.
    frame2 = cpl_frame_new();
    cpl_frame_set_filename(frame2, INPUT_FILES[1]);
    cpl_frame_set_tag(frame2, FRAME_TAGS[1]);

    // Create a multiframe container from the input files
    multiframe = cpl_multiframe_new(frame1, LABELS[0], NULL);
    cpl_test_nonnull(multiframe);

    ignore_keys_main = cpl_regex_new("(^DATAMIN|^DATAMAX|ESO QC.*)", CPL_TRUE,
                                     CPL_REGEX_EXTENDED | CPL_REGEX_NOSUBS);
    ignore_keys_ext =
        cpl_regex_new("(^BUNIT|^INHERIT|ESO DRS (DOUBLE|CHAR|STRING).*)",
                      CPL_TRUE, CPL_REGEX_EXTENDED | CPL_REGEX_NOSUBS);

    ecode = cpl_multiframe_append_dataset(multiframe, LABELS[1], frame2, "TEST",
                                          ignore_keys_main, ignore_keys_ext,
                                          CPL_MULTIFRAME_ID_JOIN);
    cpl_test_eq_error(ecode, CPL_ERROR_NONE);
    cpl_regex_delete(ignore_keys_ext);
    ignore_keys_ext = NULL;
    cpl_regex_delete(ignore_keys_main);
    ignore_keys_main = NULL;

    ecode = cpl_multiframe_write(multiframe, OUTPUT_FILE);
    cpl_test_eq_error(ecode, CPL_ERROR_NONE);

    // Check keyword contents of the added dataset. Verify that
    // all ignored keywords have not been compied to the new
    // dataset header.
    hdr_product = cpl_propertylist_load(OUTPUT_FILE, 1);
    cpl_test_nonnull(hdr_product);
    cpl_test_eq(cpl_propertylist_has(hdr_product, "INSTRUME"), 1);
    cpl_test_eq(cpl_propertylist_has(hdr_product, "EXPTIME"), 1);
    cpl_test_eq(cpl_propertylist_has(hdr_product, "MJD-OBS"), 1);
    cpl_test_eq(cpl_propertylist_has(hdr_product, "DATE-OBS"), 1);
    cpl_test_eq(cpl_propertylist_has(hdr_product, "BUNIT"), 1);
    cpl_test_eq(cpl_propertylist_has(hdr_product, "BLANK"), 1);
    cpl_test_eq(cpl_propertylist_has(hdr_product, "DATAMIN"), 1);
    cpl_test_eq(cpl_propertylist_has(hdr_product, "DATAMAX"), 1);
    cpl_test_zero(cpl_propertylist_has(hdr_product, "INHERIT"));
    cpl_test_zero(cpl_propertylist_has(hdr_product, "ESO QC LOGICAL VALUE0"));
    cpl_test_zero(cpl_propertylist_has(hdr_product, "ESO QC LOGICAL VALUE1"));
    cpl_test_zero(cpl_propertylist_has(hdr_product, "ESO QC LOGICAL VALUE2"));
    cpl_test_zero(cpl_propertylist_has(hdr_product, "ESO QC INTEGER VALUE0"));
    cpl_test_zero(cpl_propertylist_has(hdr_product, "ESO QC INTEGER VALUE1"));
    cpl_test_zero(cpl_propertylist_has(hdr_product, "ESO QC INTEGER VALUE2"));
    cpl_test_zero(cpl_propertylist_has(hdr_product, "ESO QC FLOAT VALUE0"));
    cpl_test_zero(cpl_propertylist_has(hdr_product, "ESO QC FLOAT VALUE1"));
    cpl_test_zero(cpl_propertylist_has(hdr_product, "ESO QC FLOAT VALUE2"));
    cpl_test_zero(cpl_propertylist_has(hdr_product, "ESO QC COMPLEX VALUE0"));
    cpl_test_zero(cpl_propertylist_has(hdr_product, "ESO QC COMPLEX VALUE1"));
    cpl_test_zero(cpl_propertylist_has(hdr_product, "ESO QC COMPLEX VALUE2"));
    cpl_test_zero(cpl_propertylist_has(hdr_product, "ESO QC CHAR VALUE0"));
    cpl_test_zero(cpl_propertylist_has(hdr_product, "ESO QC CHAR VALUE1"));
    cpl_test_zero(cpl_propertylist_has(hdr_product, "ESO QC CHAR VALUE2"));
    cpl_test_zero(cpl_propertylist_has(hdr_product, "ESO QC STRING VALUE0"));
    cpl_test_zero(cpl_propertylist_has(hdr_product, "ESO QC STRING VALUE1"));
    cpl_test_zero(cpl_propertylist_has(hdr_product, "ESO QC STRING VALUE2"));
    cpl_test_zero(cpl_propertylist_has(hdr_product, "ESO DRS DOUBLE VALUE"));
    cpl_test_zero(cpl_propertylist_has(hdr_product, "ESO DRS CHAR VALUE"));
    cpl_test_zero(cpl_propertylist_has(hdr_product, "ESO DRS STRING VALUE"));

    snprintf(TEST_BUFFER, BUFFER_SIZE, "%s%s%s", "PRIMARY_HDU", LABELS[1],
             "TEST");
    cpl_test_eq(strncmp(cpl_propertylist_get_string(hdr_product, "EXTNAME"),
                        TEST_BUFFER, BUFFER_SIZE),
                0);
    cpl_test_eq(strncmp(cpl_multiframe_dataset_get_id(multiframe, 1),
                        TEST_BUFFER, BUFFER_SIZE),
                0);
    TEST_BUFFER[0] = '\0';
    cpl_test_eq(strncmp(cpl_propertylist_get_string(hdr_product, "BUNIT"),
                        "counts", 6),
                0);
    cpl_test_eq(cpl_propertylist_get_float(hdr_product, "DATAMIN"), -1.);
    cpl_test_eq(cpl_propertylist_get_float(hdr_product, "DATAMAX"), 0.);
    cpl_propertylist_delete(hdr_product);

    cpl_multiframe_delete(multiframe);
    multiframe = NULL;

    // Test 10: Append data group by position
    // FIXME: TBD

    // Test 11: Append data group by name
    // FIXME: TBD

    // Test 12: Remove keywords from a dataset in a multiframe container.
    cpl_propertylist *remove_properties = cpl_propertylist_new();
    cpl_test_nonnull(remove_properties);

    // Create a multiframe container from the input files
    multiframe = cpl_multiframe_new(frame1, LABELS[0], NULL);
    cpl_test_nonnull(multiframe);

    ignore_keys_main = cpl_regex_new("(^DATAMIN|^DATAMAX|ESO QC.*)", CPL_TRUE,
                                     CPL_REGEX_EXTENDED | CPL_REGEX_NOSUBS);

    ecode = cpl_multiframe_append_dataset(multiframe, LABELS[1], frame2, "TEST",
                                          ignore_keys_main, NULL,
                                          CPL_MULTIFRAME_ID_SET);
    cpl_test_eq_error(ecode, CPL_ERROR_NONE);
    cpl_regex_delete(ignore_keys_main);
    ignore_keys_main = NULL;

    // Test 12a: Remove and empty property list.
    ecode = cpl_multiframe_dataset_properties_remove(multiframe, 1,
                                                     remove_properties);
    cpl_test_eq_error(ecode, CPL_ERROR_NONE);

    // Test 12b: Remove some primary FITS records. Note that the type of the
    // property or the value it has in the list of properties to remove does
    // not matter properties are only matched by name. Also, if a property
    // appears more than once, then it is removed as often as it appears.
    cpl_propertylist_update_bool(remove_properties, "INHERIT", CPL_FALSE);
    cpl_propertylist_update_int(remove_properties, "BLANK", -1);
    cpl_propertylist_update_float(remove_properties, "EXPTIME", 0.);
    cpl_propertylist_update_double(remove_properties, "MJD-OBS", 0.);
    cpl_propertylist_update_string(remove_properties, "BUNIT", "");
    cpl_propertylist_update_string(remove_properties, "DATE-OBS", "");
    cpl_propertylist_update_bool(remove_properties, "ESO DRS LOGICAL VALUE",
                                 CPL_TRUE);
    cpl_propertylist_update_int(remove_properties, "ESO DRS INT VALUE", 0);
    cpl_propertylist_update_float(remove_properties, "ESO DRS FLOAT VALUE", 0.);
    cpl_propertylist_update_double(remove_properties, "ESO DRS DOUBLE VALUE",
                                   0.);
    cpl_propertylist_update_double_complex(remove_properties,
                                           "ESO DRS COMPLEX VALUE", 0.);
    cpl_propertylist_update_string(remove_properties, "ESO DRS STRING VALUE",
                                   "");

    cpl_propertylist_append_string(remove_properties, "BUNIT", "");
    cpl_propertylist_append_string(remove_properties, "BUNIT", "");

    ecode = cpl_multiframe_dataset_properties_remove(multiframe, 1,
                                                     remove_properties);
    cpl_test_eq_error(ecode, CPL_ERROR_NONE);

    cpl_propertylist_delete(remove_properties);
    remove_properties = NULL;

    ecode = cpl_multiframe_write(multiframe, OUTPUT_FILE);
    cpl_test_eq_error(ecode, CPL_ERROR_NONE);

    // Check keyword contents of the added dataset.
    hdr_product = cpl_propertylist_load(OUTPUT_FILE, 1);
    cpl_test_nonnull(hdr_product);
    cpl_test_zero(cpl_propertylist_has(hdr_product, "INHERIT"));
    cpl_test_zero(cpl_propertylist_has(hdr_product, "BLANK"));
    cpl_test_zero(cpl_propertylist_has(hdr_product, "EXPTIME"));
    cpl_test_zero(cpl_propertylist_has(hdr_product, "MJD-OBS"));
    cpl_test_zero(cpl_propertylist_has(hdr_product, "DATE-OBS"));
    cpl_test_zero(cpl_propertylist_has(hdr_product, "BUNIT"));
    cpl_propertylist_delete(hdr_product);

    cpl_multiframe_delete(multiframe);
    multiframe = NULL;

    // Test 13: Add keywords from to dataset in a multiframe container.
    cpl_propertylist *add_properties = cpl_propertylist_new();
    cpl_test_nonnull(add_properties);

    // Create a multiframe container from the input files
    multiframe = cpl_multiframe_new(frame1, LABELS[0], NULL);
    cpl_test_nonnull(multiframe);

    ignore_keys_main =
        cpl_regex_new("(^BLANK|^DATAMIN|^DATAMAX|ESO QC.*)", CPL_TRUE,
                      CPL_REGEX_EXTENDED | CPL_REGEX_NOSUBS);
    ignore_keys_ext =
        cpl_regex_new("(^BUNIT|^BLANK|^INHERIT|^DATAMIN|^DATAMAX|ESO DRS .*)",
                      CPL_TRUE, CPL_REGEX_EXTENDED | CPL_REGEX_NOSUBS);

    ecode = cpl_multiframe_append_dataset(multiframe, LABELS[1], frame2, "TEST",
                                          ignore_keys_main, ignore_keys_ext,
                                          CPL_MULTIFRAME_ID_SET);
    cpl_test_eq_error(ecode, CPL_ERROR_NONE);
    cpl_regex_delete(ignore_keys_ext);
    ignore_keys_ext = NULL;
    cpl_regex_delete(ignore_keys_main);
    ignore_keys_main = NULL;

    // Test 13a: Check error handling
    ecode = cpl_multiframe_dataset_properties_update(multiframe, 1, NULL);
    cpl_test_eq_error(ecode, CPL_ERROR_NULL_INPUT);
    ecode =
        cpl_multiframe_dataset_properties_update(multiframe, 0, add_properties);
    cpl_test_eq_error(ecode, CPL_ERROR_NONE);
    ecode = cpl_multiframe_dataset_properties_update(multiframe, -1,
                                                     add_properties);
    cpl_test_eq_error(ecode, CPL_ERROR_ILLEGAL_INPUT);
    ecode =
        cpl_multiframe_dataset_properties_update(multiframe, 2, add_properties);
    cpl_test_eq_error(ecode, CPL_ERROR_ILLEGAL_INPUT);

    // Test 13b: Add new keywords to a dataset in a multiframe container
    cpl_propertylist_update_bool(add_properties, "INHERIT", CPL_TRUE);
    cpl_propertylist_set_comment(add_properties, "INHERIT",
                                 "T if primary header keywords are inherited");
    cpl_propertylist_update_char(add_properties, "CHARVAL", CHAR_VALUES[0]);
    cpl_propertylist_update_int(add_properties, "INTVAL", INT_VALUES[0]);
    cpl_propertylist_update_float(add_properties, "FLOATVAL", FLOAT_VALUES[0]);
    cpl_propertylist_update_double(add_properties, "DBLEVAL", FLOAT_VALUES[0]);
    cpl_propertylist_update_double_complex(add_properties, "CMPLXVAL",
                                           CMPLX_VALUES[0]);
    cpl_propertylist_update_string(add_properties, "STRVAL", STRING_VALUES[2]);

    cpl_propertylist_update_bool(add_properties, "ESO DRS LOGICAL VALUE",
                                 CPL_TRUE);
    cpl_propertylist_set_comment(add_properties, "ESO DRS LOGICAL VALUE",
                                 "T if the value is true and F otherwise");
    cpl_propertylist_update_char(add_properties, "ESO DRS CHAR VALUE",
                                 CHAR_VALUES[0]);
    cpl_propertylist_update_int(add_properties, "ESO DRS INT VALUE",
                                INT_VALUES[0]);
    cpl_propertylist_update_float(add_properties, "ESO DRS FLOAT VALUE",
                                  FLOAT_VALUES[0]);
    cpl_propertylist_update_double(add_properties, "ESO DRS DOUBLE VALUE",
                                   FLOAT_VALUES[0]);
    cpl_propertylist_update_double_complex(add_properties,
                                           "ESO DRS COMPLEX VALUE",
                                           CMPLX_VALUES[0]);
    cpl_propertylist_update_string(add_properties, "ESO DRS STRING VALUE",
                                   STRING_VALUES[2]);

    ecode =
        cpl_multiframe_dataset_properties_update(multiframe, 1, add_properties);
    cpl_test_eq_error(ecode, CPL_ERROR_NONE);

    ecode = cpl_multiframe_write(multiframe, OUTPUT_FILE);
    cpl_test_eq_error(ecode, CPL_ERROR_NONE);

    // Check keyword contents of the added dataset.
    hdr_product = cpl_propertylist_load(OUTPUT_FILE, 1);
    cpl_test_nonnull(hdr_product);
    cpl_test_eq(cpl_propertylist_has(hdr_product, "INHERIT"), 1);
    cpl_test_eq(cpl_propertylist_has(hdr_product, "CHARVAL"), 1);
    cpl_test_eq(cpl_propertylist_has(hdr_product, "INTVAL"), 1);
    cpl_test_eq(cpl_propertylist_has(hdr_product, "FLOATVAL"), 1);
    cpl_test_eq(cpl_propertylist_has(hdr_product, "DBLEVAL"), 1);
    cpl_test_eq(cpl_propertylist_has(hdr_product, "CMPLXVAL"), 1);
    cpl_test_eq(cpl_propertylist_has(hdr_product, "STRVAL"), 1);
    cpl_test_eq(cpl_propertylist_has(hdr_product, "ESO DRS LOGICAL VALUE"), 1);
    cpl_test_eq(cpl_propertylist_has(hdr_product, "ESO DRS CHAR VALUE"), 1);
    cpl_test_eq(cpl_propertylist_has(hdr_product, "ESO DRS INT VALUE"), 1);
    cpl_test_eq(cpl_propertylist_has(hdr_product, "ESO DRS FLOAT VALUE"), 1);
    cpl_test_eq(cpl_propertylist_has(hdr_product, "ESO DRS DOUBLE VALUE"), 1);
    cpl_test_eq(cpl_propertylist_has(hdr_product, "ESO DRS COMPLEX VALUE"), 1);
    cpl_test_eq(cpl_propertylist_has(hdr_product, "ESO DRS STRING VALUE"), 1);

    cpl_test_eq(cpl_propertylist_get_bool(hdr_product, "INHERIT"),
                cpl_propertylist_get_bool(add_properties, "INHERIT"));
    cpl_test_eq(strcmp(cpl_propertylist_get_comment(hdr_product, "INHERIT"),
                       cpl_propertylist_get_comment(add_properties, "INHERIT")),
                0);
    cpl_test_eq(cpl_propertylist_get_int(hdr_product, "INTVAL"),
                cpl_propertylist_get_int(add_properties, "INTVAL"));
    cpl_test_eq(cpl_propertylist_get_float(hdr_product, "FLOATVAL"),
                cpl_propertylist_get_float(add_properties, "FLOATVAL"));
    cpl_test_eq(cpl_propertylist_get_double(hdr_product, "DBLEVAL"),
                cpl_propertylist_get_double(add_properties, "DBLEVAL"));
    cpl_test_eq(cpl_propertylist_get_double_complex(hdr_product, "CMPLXVAL"),
                cpl_propertylist_get_double_complex(add_properties,
                                                    "CMPLXVAL"));
    cpl_test_eq(strcmp(cpl_propertylist_get_string(hdr_product, "STRVAL"),
                       cpl_propertylist_get_string(add_properties, "STRVAL")),
                0);
    // This is a special case because char properties get translated to
    // strings on writing them to a file!
    cpl_test_eq(cpl_propertylist_get_string(hdr_product, "CHARVAL")[0],
                cpl_propertylist_get_char(add_properties, "CHARVAL"));

    cpl_test_eq(cpl_propertylist_get_bool(hdr_product, "ESO DRS LOGICAL VALUE"),
                cpl_propertylist_get_bool(add_properties,
                                          "ESO DRS LOGICAL VALUE"));
    cpl_test_eq(strcmp(cpl_propertylist_get_comment(hdr_product,
                                                    "ESO DRS LOGICAL VALUE"),
                       cpl_propertylist_get_comment(add_properties,
                                                    "ESO DRS LOGICAL VALUE")),
                0);
    cpl_test_eq(cpl_propertylist_get_int(hdr_product, "ESO DRS INT VALUE"),
                cpl_propertylist_get_int(add_properties, "ESO DRS INT VALUE"));
    cpl_test_eq(cpl_propertylist_get_float(hdr_product, "ESO DRS FLOAT VALUE"),
                cpl_propertylist_get_float(add_properties,
                                           "ESO DRS FLOAT VALUE"));
    cpl_test_eq(cpl_propertylist_get_double(hdr_product,
                                            "ESO DRS DOUBLE VALUE"),
                cpl_propertylist_get_double(add_properties,
                                            "ESO DRS DOUBLE VALUE"));
    cpl_test_eq(cpl_propertylist_get_double_complex(hdr_product,
                                                    "ESO DRS COMPLEX VALUE"),
                cpl_propertylist_get_double_complex(add_properties,
                                                    "ESO DRS COMPLEX VALUE"));
    cpl_test_eq(strcmp(cpl_propertylist_get_string(hdr_product,
                                                   "ESO DRS STRING VALUE"),
                       cpl_propertylist_get_string(add_properties,
                                                   "ESO DRS STRING VALUE")),
                0);
    // This is a special case because char properties get translated to
    // strings on writing them to a file!
    cpl_test_eq(cpl_propertylist_get_string(hdr_product,
                                            "ESO DRS CHAR VALUE")[0],
                cpl_propertylist_get_char(add_properties,
                                          "ESO DRS CHAR VALUE"));
    cpl_propertylist_delete(hdr_product);

    // Test 13c: Update exisitng keywords of a dataset in a multiframe container
    cpl_propertylist_update_string(add_properties, "BUNIT",
                                   "10**(-20)*erg/s/cm**2/Angstrom");
    cpl_propertylist_set_comment(add_properties, "BUNIT", "Flux units");
    cpl_propertylist_update_bool(add_properties, "INHERIT", CPL_FALSE);
    cpl_propertylist_set_comment(
        add_properties, "INHERIT",
        "F if primary header keywords are not inherited");
    cpl_propertylist_update_char(add_properties, "CHARVAL", CHAR_VALUES[2]);
    cpl_propertylist_set_comment(add_properties, "CHARVAL",
                                 "A character value keyword");
    cpl_propertylist_update_int(add_properties, "INTVAL", INT_VALUES[2]);
    cpl_propertylist_set_comment(add_properties, "INTVAL",
                                 "An integer value keyword");
    cpl_propertylist_update_float(add_properties, "FLOATVAL", FLOAT_VALUES[2]);
    cpl_propertylist_set_comment(add_properties, "FLOATVAL",
                                 "A floating-point value keyword");
    cpl_propertylist_update_double(add_properties, "DBLEVAL", FLOAT_VALUES[2]);
    cpl_propertylist_set_comment(
        add_properties, "DBLEVAL",
        "A double precision floating-point value keyword");
    cpl_propertylist_update_double_complex(add_properties, "CMPLXVAL",
                                           CMPLX_VALUES[2]);
    cpl_propertylist_set_comment(add_properties, "CMPLXVAL",
                                 "A double precision complex value keyword");
    cpl_propertylist_update_string(add_properties, "STRVAL", STRING_VALUES[0]);
    cpl_propertylist_set_comment(add_properties, "STRVAL",
                                 "A string value keyword");

    cpl_propertylist_update_bool(add_properties, "ESO DRS LOGICAL VALUE",
                                 CPL_FALSE);
    cpl_propertylist_set_comment(add_properties, "ESO DRS LOGICAL VALUE",
                                 "A short comment");
    cpl_propertylist_update_char(add_properties, "ESO DRS CHAR VALUE",
                                 CHAR_VALUES[2]);
    cpl_propertylist_set_comment(add_properties, "ESO DRS CHAR VALUE",
                                 "A short comment");
    cpl_propertylist_update_int(add_properties, "ESO DRS INT VALUE",
                                INT_VALUES[2]);
    cpl_propertylist_set_comment(add_properties, "ESO DRS INT VALUE",
                                 "A short comment");
    cpl_propertylist_update_float(add_properties, "ESO DRS FLOAT VALUE",
                                  FLOAT_VALUES[2]);
    cpl_propertylist_set_comment(add_properties, "ESO DRS FLOAT VALUE",
                                 "A short comment");
    cpl_propertylist_update_double(add_properties, "ESO DRS DOUBLE VALUE",
                                   FLOAT_VALUES[2]);
    cpl_propertylist_set_comment(add_properties, "ESO DRS DOUBLE VALUE",
                                 "A short comment");
    cpl_propertylist_update_double_complex(add_properties,
                                           "ESO DRS COMPLEX VALUE",
                                           CMPLX_VALUES[2]);
    cpl_propertylist_set_comment(add_properties, "ESO DRS COMPLEX VALUE",
                                 "A short comment");
    cpl_propertylist_update_string(add_properties, "ESO DRS STRING VALUE",
                                   STRING_VALUES[0]);
    cpl_propertylist_set_comment(add_properties, "ESO DRS STRING VALUE",
                                 "A short comment");

    ecode =
        cpl_multiframe_dataset_properties_update(multiframe, 1, add_properties);
    cpl_test_eq_error(ecode, CPL_ERROR_NONE);

    ecode = cpl_multiframe_write(multiframe, OUTPUT_FILE);
    cpl_test_eq_error(ecode, CPL_ERROR_NONE);

    // Check keyword contents of the added dataset.
    hdr_product = cpl_propertylist_load(OUTPUT_FILE, 1);
    cpl_test_nonnull(hdr_product);
    cpl_test_eq(cpl_propertylist_has(hdr_product, "BUNIT"), 1);
    cpl_test_eq(cpl_propertylist_has(hdr_product, "INHERIT"), 1);
    cpl_test_eq(cpl_propertylist_has(hdr_product, "CHARVAL"), 1);
    cpl_test_eq(cpl_propertylist_has(hdr_product, "INTVAL"), 1);
    cpl_test_eq(cpl_propertylist_has(hdr_product, "FLOATVAL"), 1);
    cpl_test_eq(cpl_propertylist_has(hdr_product, "DBLEVAL"), 1);
    cpl_test_eq(cpl_propertylist_has(hdr_product, "CMPLXVAL"), 1);
    cpl_test_eq(cpl_propertylist_has(hdr_product, "STRVAL"), 1);
    cpl_test_eq(cpl_propertylist_has(hdr_product, "ESO DRS LOGICAL VALUE"), 1);
    cpl_test_eq(cpl_propertylist_has(hdr_product, "ESO DRS CHAR VALUE"), 1);
    cpl_test_eq(cpl_propertylist_has(hdr_product, "ESO DRS INT VALUE"), 1);
    cpl_test_eq(cpl_propertylist_has(hdr_product, "ESO DRS FLOAT VALUE"), 1);
    cpl_test_eq(cpl_propertylist_has(hdr_product, "ESO DRS DOUBLE VALUE"), 1);
    cpl_test_eq(cpl_propertylist_has(hdr_product, "ESO DRS COMPLEX VALUE"), 1);
    cpl_test_eq(cpl_propertylist_has(hdr_product, "ESO DRS STRING VALUE"), 1);

    cpl_test_eq(strcmp(cpl_propertylist_get_string(hdr_product, "BUNIT"),
                       cpl_propertylist_get_string(add_properties, "BUNIT")),
                0);
    cpl_test_eq(strcmp(cpl_propertylist_get_comment(hdr_product, "BUNIT"),
                       cpl_propertylist_get_comment(add_properties, "BUNIT")),
                0);
    cpl_test_eq(cpl_propertylist_get_bool(hdr_product, "INHERIT"),
                cpl_propertylist_get_bool(add_properties, "INHERIT"));
    cpl_test_eq(strcmp(cpl_propertylist_get_comment(hdr_product, "INHERIT"),
                       cpl_propertylist_get_comment(add_properties, "INHERIT")),
                0);
    // This is a special case because char properties get translated to
    // strings on writing them to a file!
    cpl_test_eq(cpl_propertylist_get_string(hdr_product, "CHARVAL")[0],
                cpl_propertylist_get_char(add_properties, "CHARVAL"));
    cpl_test_eq(strcmp(cpl_propertylist_get_comment(hdr_product, "CHARVAL"),
                       cpl_propertylist_get_comment(add_properties, "CHARVAL")),
                0);
    cpl_test_eq(cpl_propertylist_get_int(hdr_product, "INTVAL"),
                cpl_propertylist_get_int(add_properties, "INTVAL"));
    cpl_test_eq(strcmp(cpl_propertylist_get_comment(hdr_product, "INTVAL"),
                       cpl_propertylist_get_comment(add_properties, "INTVAL")),
                0);
    cpl_test_eq(cpl_propertylist_get_float(hdr_product, "FLOATVAL"),
                cpl_propertylist_get_float(add_properties, "FLOATVAL"));
    cpl_test_eq(strcmp(cpl_propertylist_get_comment(hdr_product, "FLOATVAL"),
                       cpl_propertylist_get_comment(add_properties,
                                                    "FLOATVAL")),
                0);
    cpl_test_eq(cpl_propertylist_get_double(hdr_product, "DBLEVAL"),
                cpl_propertylist_get_double(add_properties, "DBLEVAL"));
    cpl_test_eq(strcmp(cpl_propertylist_get_comment(hdr_product, "DBLEVAL"),
                       cpl_propertylist_get_comment(add_properties, "DBLEVAL")),
                0);
    cpl_test_eq(cpl_propertylist_get_double_complex(hdr_product, "CMPLXVAL"),
                cpl_propertylist_get_double_complex(add_properties,
                                                    "CMPLXVAL"));
    cpl_test_eq(strcmp(cpl_propertylist_get_comment(hdr_product, "CMPLXVAL"),
                       cpl_propertylist_get_comment(add_properties,
                                                    "CMPLXVAL")),
                0);
    cpl_test_eq(strcmp(cpl_propertylist_get_string(hdr_product, "STRVAL"),
                       cpl_propertylist_get_string(add_properties, "STRVAL")),
                0);
    cpl_test_eq(strcmp(cpl_propertylist_get_comment(hdr_product, "STRVAL"),
                       cpl_propertylist_get_comment(add_properties, "STRVAL")),
                0);

    // This is a special case because char properties get translated to
    // strings on writing them to a file!
    cpl_test_eq(cpl_propertylist_get_string(hdr_product,
                                            "ESO DRS CHAR VALUE")[0],
                cpl_propertylist_get_char(add_properties,
                                          "ESO DRS CHAR VALUE"));
    cpl_test_eq(strcmp(cpl_propertylist_get_comment(hdr_product,
                                                    "ESO DRS CHAR VALUE"),
                       cpl_propertylist_get_comment(add_properties,
                                                    "ESO DRS CHAR VALUE")),
                0);
    cpl_test_eq(cpl_propertylist_get_bool(hdr_product, "ESO DRS LOGICAL VALUE"),
                cpl_propertylist_get_bool(add_properties,
                                          "ESO DRS LOGICAL VALUE"));
    cpl_test_eq(strcmp(cpl_propertylist_get_comment(hdr_product,
                                                    "ESO DRS LOGICAL VALUE"),
                       cpl_propertylist_get_comment(add_properties,
                                                    "ESO DRS LOGICAL VALUE")),
                0);
    cpl_test_eq(cpl_propertylist_get_int(hdr_product, "ESO DRS INT VALUE"),
                cpl_propertylist_get_int(add_properties, "ESO DRS INT VALUE"));
    cpl_test_eq(strcmp(cpl_propertylist_get_comment(hdr_product,
                                                    "ESO DRS INT VALUE"),
                       cpl_propertylist_get_comment(add_properties,
                                                    "ESO DRS INT VALUE")),
                0);
    cpl_test_eq(cpl_propertylist_get_float(hdr_product, "ESO DRS FLOAT VALUE"),
                cpl_propertylist_get_float(add_properties,
                                           "ESO DRS FLOAT VALUE"));
    cpl_test_eq(strcmp(cpl_propertylist_get_comment(hdr_product,
                                                    "ESO DRS FLOAT VALUE"),
                       cpl_propertylist_get_comment(add_properties,
                                                    "ESO DRS FLOAT VALUE")),
                0);
    cpl_test_eq(cpl_propertylist_get_double(hdr_product,
                                            "ESO DRS DOUBLE VALUE"),
                cpl_propertylist_get_double(add_properties,
                                            "ESO DRS DOUBLE VALUE"));
    cpl_test_eq(strcmp(cpl_propertylist_get_comment(hdr_product,
                                                    "ESO DRS DOUBLE VALUE"),
                       cpl_propertylist_get_comment(add_properties,
                                                    "ESO DRS DOUBLE VALUE")),
                0);
    cpl_test_eq(cpl_propertylist_get_double_complex(hdr_product,
                                                    "ESO DRS COMPLEX VALUE"),
                cpl_propertylist_get_double_complex(add_properties,
                                                    "ESO DRS COMPLEX VALUE"));
    cpl_test_eq(strcmp(cpl_propertylist_get_comment(hdr_product,
                                                    "ESO DRS COMPLEX VALUE"),
                       cpl_propertylist_get_comment(add_properties,
                                                    "ESO DRS COMPLEX VALUE")),
                0);
    cpl_test_eq(strcmp(cpl_propertylist_get_string(hdr_product,
                                                   "ESO DRS STRING VALUE"),
                       cpl_propertylist_get_string(add_properties,
                                                   "ESO DRS STRING VALUE")),
                0);
    cpl_test_eq(strcmp(cpl_propertylist_get_comment(hdr_product,
                                                    "ESO DRS STRING VALUE"),
                       cpl_propertylist_get_comment(add_properties,
                                                    "ESO DRS STRING VALUE")),
                0);
    cpl_propertylist_delete(hdr_product);

    cpl_propertylist_delete(add_properties);
    add_properties = NULL;

    // Test 13d: Trigger some truncation cases. Comments will be silently
    // truncated, but too long string values will not. It will raise an error.
    add_properties = cpl_propertylist_new();
    cpl_propertylist_update_double(add_properties, "ESO DRS DOUBLE VALUE",
                                   FLOAT_VALUES[2]);
    cpl_propertylist_set_comment(
        add_properties, "ESO DRS DOUBLE VALUE",
        "A double precision floating-point value hierarchical keyword");
    cpl_propertylist_update_string(add_properties, "ESO DRS STRING VALUE",
                                   STRING_VALUES[3]);
    cpl_propertylist_set_comment(add_properties, "ESO DRS STRING VALUE",
                                 "A string value hierarchical keyword with a "
                                 "long value and a long comment");

    ecode =
        cpl_multiframe_dataset_properties_update(multiframe, 1, add_properties);
    cpl_test_eq_error(ecode, CPL_ERROR_ILLEGAL_OUTPUT);

    ecode = cpl_multiframe_write(multiframe, OUTPUT_FILE);
    cpl_test_eq_error(ecode, CPL_ERROR_NONE);

    // Check keyword contents of the added dataset.
    hdr_product = cpl_propertylist_load(OUTPUT_FILE, 1);
    cpl_test_nonnull(hdr_product);
    cpl_test_eq(cpl_propertylist_has(hdr_product, "ESO DRS DOUBLE VALUE"), 1);
    cpl_test_eq(cpl_propertylist_has(hdr_product, "ESO DRS STRING VALUE"), 1);

    cpl_test_eq(cpl_propertylist_get_double(hdr_product,
                                            "ESO DRS DOUBLE VALUE"),
                cpl_propertylist_get_double(add_properties,
                                            "ESO DRS DOUBLE VALUE"));
    cpl_test_noneq(strcmp(cpl_propertylist_get_comment(hdr_product,
                                                       "ESO DRS DOUBLE VALUE"),
                          cpl_propertylist_get_comment(add_properties,
                                                       "ESO DRS DOUBLE VALUE")),
                   0);
    cpl_test_eq(strncmp(cpl_propertylist_get_comment(hdr_product,
                                                     "ESO DRS DOUBLE VALUE"),
                        cpl_propertylist_get_comment(add_properties,
                                                     "ESO DRS DOUBLE VALUE"),
                        39),
                0);
    cpl_test_noneq(strcmp(cpl_propertylist_get_string(hdr_product,
                                                      "ESO DRS STRING VALUE"),
                          cpl_propertylist_get_string(add_properties,
                                                      "ESO DRS STRING VALUE")),
                   0);
    // The property of the dataset in the multiframe container should be
    // untouched. It therefore should still have its previous value from the
    // preceeding test.
    cpl_test_eq(strcmp(cpl_propertylist_get_string(hdr_product,
                                                   "ESO DRS STRING VALUE"),
                       STRING_VALUES[0]),
                0);
    cpl_test_noneq(strcmp(cpl_propertylist_get_comment(hdr_product,
                                                       "ESO DRS STRING VALUE"),
                          cpl_propertylist_get_comment(add_properties,
                                                       "ESO DRS STRING VALUE")),
                   0);
    cpl_propertylist_delete(hdr_product);

    cpl_propertylist_delete(add_properties);
    add_properties = NULL;

    // Test 13e: Verify keyword type checking when updating existing keywords.
    // It is possible to overwrite keywords with the same type class.
    add_properties = cpl_propertylist_new();
    cpl_propertylist_update_string(add_properties, "ESO DRS CHAR VALUE",
                                   STRING_VALUES[1]);
    cpl_propertylist_update_char(add_properties, "ESO DRS STRING VALUE",
                                 CHAR_VALUES[1]);
    cpl_propertylist_update_float(add_properties, "ESO DRS DOUBLE VALUE",
                                  FLOAT_VALUES[1]);
    cpl_propertylist_update_double(add_properties, "ESO DRS FLOAT VALUE",
                                   FLOAT_VALUES[1]);
    cpl_propertylist_update_float_complex(add_properties,
                                          "ESO DRS COMPLEX VALUE",
                                          CMPLX_VALUES[1]);
    cpl_propertylist_update_long_long(add_properties, "ESO DRS INT VALUE",
                                      INT_VALUES[1]);
    ecode =
        cpl_multiframe_dataset_properties_update(multiframe, 1, add_properties);
    cpl_test_eq_error(ecode, CPL_ERROR_NONE);

    cpl_propertylist_empty(add_properties);
    cpl_propertylist_update_bool(add_properties, "ESO DRS CHAR VALUE",
                                 CPL_TRUE);
    ecode =
        cpl_multiframe_dataset_properties_update(multiframe, 1, add_properties);
    cpl_test_eq_error(ecode, CPL_ERROR_TYPE_MISMATCH);

    cpl_propertylist_empty(add_properties);
    cpl_propertylist_update_int(add_properties, "ESO DRS LOGICAL VALUE",
                                INT_VALUES[1]);
    ecode =
        cpl_multiframe_dataset_properties_update(multiframe, 1, add_properties);
    cpl_test_eq_error(ecode, CPL_ERROR_TYPE_MISMATCH);

    cpl_propertylist_empty(add_properties);
    cpl_propertylist_update_float(add_properties, "ESO DRS INT VALUE",
                                  FLOAT_VALUES[1]);
    ecode =
        cpl_multiframe_dataset_properties_update(multiframe, 1, add_properties);
    cpl_test_eq_error(ecode, CPL_ERROR_TYPE_MISMATCH);

    cpl_propertylist_empty(add_properties);
    cpl_propertylist_update_int(add_properties, "ESO DRS FLOAT VALUE",
                                INT_VALUES[1]);
    ecode =
        cpl_multiframe_dataset_properties_update(multiframe, 1, add_properties);
    cpl_test_eq_error(ecode, CPL_ERROR_TYPE_MISMATCH);

    cpl_propertylist_empty(add_properties);
    cpl_propertylist_update_double(add_properties, "ESO DRS COMPLEX VALUE",
                                   FLOAT_VALUES[1]);
    ecode =
        cpl_multiframe_dataset_properties_update(multiframe, 1, add_properties);
    cpl_test_eq_error(ecode, CPL_ERROR_TYPE_MISMATCH);

    cpl_propertylist_empty(add_properties);
    cpl_propertylist_update_float_complex(add_properties,
                                          "ESO DRS STRING VALUE",
                                          CMPLX_VALUES[1]);
    ecode =
        cpl_multiframe_dataset_properties_update(multiframe, 1, add_properties);
    cpl_test_eq_error(ecode, CPL_ERROR_TYPE_MISMATCH);

    cpl_propertylist_delete(add_properties);
    add_properties = NULL;

    cpl_multiframe_delete(multiframe);
    multiframe = NULL;

    // Cleanup
    cpl_frame_delete(frame1);
    cpl_frame_delete(frame2);
    cpl_propertylist_delete(hdr_ext);
    cpl_propertylist_delete(hdr_main);

    // Tests finished
    cpl_test_end(0);

    return 0;
}