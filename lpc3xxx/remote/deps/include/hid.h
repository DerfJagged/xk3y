#ifndef __INCLUDED_HID_H__
#define __INCLUDED_HID_H__

#include <stdio.h>
#include <usb.h>
#include <hidparser.h>

/*!@file
 * @brief Main libhid API header file
 *
 * This header file forms the public API for libhid. Functions not prototyped
 * here are most likely for internal use only, and may change without notice.
 */

#ifndef byte
  typedef unsigned char byte;
#endif

#ifdef HAVE_STDBOOL_H
#  include <stdbool.h>
#else
#  define bool  _Bool
#  define true  1
#  define false 0
#endif

typedef enum hid_return_t {
  HID_RET_SUCCESS = 0,
  HID_RET_INVALID_PARAMETER,
  HID_RET_NOT_INITIALISED,
  HID_RET_ALREADY_INITIALISED,
  HID_RET_FAIL_FIND_BUSSES,
  HID_RET_FAIL_FIND_DEVICES,
  HID_RET_FAIL_OPEN_DEVICE,
  HID_RET_DEVICE_NOT_FOUND,
  HID_RET_DEVICE_NOT_OPENED,
  HID_RET_DEVICE_ALREADY_OPENED,
  HID_RET_FAIL_CLOSE_DEVICE,
  HID_RET_FAIL_CLAIM_IFACE,
  HID_RET_FAIL_DETACH_DRIVER,
  HID_RET_NOT_HID_DEVICE,
  HID_RET_HID_DESC_SHORT,
  HID_RET_REPORT_DESC_SHORT,
  HID_RET_REPORT_DESC_LONG,
  HID_RET_FAIL_ALLOC,
  HID_RET_OUT_OF_SPACE,
  HID_RET_FAIL_SET_REPORT,
  HID_RET_FAIL_GET_REPORT,
  HID_RET_FAIL_INT_READ,
  HID_RET_NOT_FOUND,
  HID_RET_TIMEOUT
} hid_return;

struct usb_dev_handle;

/*!@brief Interface description
 *
 * This structure contains information associated with a given USB device
 * interface. The identification information allows multiple HID-class
 * interfaces to be accessed on a single device.
 *
 * Also available are raw and parsed descriptor information.
 */
typedef struct HIDInterface_t {
  struct usb_dev_handle *dev_handle;
  struct usb_device *device;
  int interface;
  char id[32];
  HIDData* hid_data;
  HIDParser* hid_parser;
} HIDInterface;

typedef bool (*matcher_fn_t)(struct usb_dev_handle const* usbdev,
    void* custom, unsigned int len);

typedef struct HIDInterfaceMatcher_t {
  unsigned short vendor_id;
  unsigned short product_id;
#ifndef SWIG
  matcher_fn_t matcher_fn;	//!< Only supported in C library (not via SWIG)
  void* custom_data;		   //!< Only used by matcher_fn
  unsigned int custom_data_length; //!< Only used by matcher_fn
#endif
} HIDInterfaceMatcher;

#define HID_ID_MATCH_ANY 0x0000

/*!@brief Bitmask for selection of debugging messages
 *
 * The values of this enumeration can be combined with the bitwise-OR operator
 * to select which debug messages should be printed. The selection can be set
 * with hid_set_debug(). You can set a file descriptor for error messages with
 * hid_set_debug_stream().
 */
typedef enum HIDDebugLevel_t {
  HID_DEBUG_NONE = 0x0,		//!< Default
  HID_DEBUG_ERRORS = 0x1,	//!< Serious conditions
  HID_DEBUG_WARNINGS = 0x2,	//!< Less serious conditions
  HID_DEBUG_NOTICES = 0x4,	//!< Informational messages
  HID_DEBUG_TRACES = 0x8,	//!< Verbose tracing of functions
  HID_DEBUG_ASSERTS = 0x10,	//!< Assertions for sanity checking
  HID_DEBUG_NOTRACES = HID_DEBUG_ERRORS | HID_DEBUG_WARNINGS | HID_DEBUG_NOTICES | HID_DEBUG_ASSERTS,
				//!< This is what you probably want to start with while developing with libhid
  HID_DEBUG_ALL = HID_DEBUG_ERRORS | HID_DEBUG_WARNINGS | HID_DEBUG_NOTICES | HID_DEBUG_TRACES | HID_DEBUG_ASSERTS
} HIDDebugLevel;
  
#ifdef __cplusplus
extern "C" {
#endif

void hid_set_debug(HIDDebugLevel const level);
void hid_set_debug_stream(FILE* const outstream);
void hid_set_usb_debug(int const level);

HIDInterface* hid_new_HIDInterface();

void hid_delete_HIDInterface(HIDInterface** const hidif);

void hid_reset_HIDInterface(HIDInterface* const hidif);

hid_return hid_init();

hid_return hid_cleanup();

bool hid_is_initialised();

hid_return hid_open(HIDInterface* const hidif, int const interface,
    HIDInterfaceMatcher const* const matcher);
hid_return hid_force_open(HIDInterface* const hidif, int const interface,
    HIDInterfaceMatcher const* const matcher, unsigned short retries);

hid_return hid_close(HIDInterface* const hidif);

bool hid_is_opened(HIDInterface const* const hidif);

const char *hid_strerror(hid_return ret);

hid_return hid_get_input_report(HIDInterface* const hidif, int const path[],
    unsigned int const depth, char* const buffer, unsigned int const size);

hid_return hid_set_output_report(HIDInterface* const hidif, int const path[],
    unsigned int const depth, char const* const buffer, unsigned int const size);

hid_return hid_get_feature_report(HIDInterface* const hidif, int const path[],
    unsigned int const depth, char* const buffer, unsigned int const size);

hid_return hid_set_feature_report(HIDInterface* const hidif, int const path[],
    unsigned int const depth, char const* const buffer, unsigned int const size);

hid_return hid_get_item_value(HIDInterface* const hidif,
                              int const path[], unsigned int const depth,
                              double *const value);

/*
hid_return hid_get_item_string(HIDInterface* const hidif,
                               int const path[], unsigned int const depth,
                               char *const value, unsigned int const maxlen);

hid_return hid_set_item_value(HIDInterface* const hidif,
                              int const path[], unsigned int const depth,
                              double const value);
*/
hid_return hid_write_identification(FILE* const out,
    HIDInterface const* const hidif);

hid_return hid_dump_tree(FILE* const out, HIDInterface* const hidif);

hid_return hid_interrupt_read(HIDInterface* const hidif, unsigned int const ep,
    char* const bytes, unsigned int const size, unsigned int const timeout);

hid_return hid_interrupt_write(HIDInterface* const hidif, unsigned int const ep,
    const char* const bytes, unsigned int const size, unsigned int const timeout);

hid_return hid_set_idle(HIDInterface * const hidif,
    unsigned duration, unsigned report_id);


#ifdef __cplusplus
}
#endif

#endif /* __INCLUDED_HID_H__ */

/* COPYRIGHT --
 *
 * This file is part of libhid, a user-space HID access library.
 * libhid is (c) 2003-2006
 *   Martin F. Krafft <libhid@pobox.madduck.net>
 *   Charles Lepple <clepple+libhid@ghz.cc>
 *   Arnaud Quette <arnaud.quette@free.fr> && <arnaud.quette@mgeups.com>
 * and distributed under the terms of the GNU General Public License.
 * See the file ./COPYING in the source distribution for more information.
 *
 * THIS PACKAGE IS PROVIDED "AS IS" AND WITHOUT ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES
 * OF MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 */
