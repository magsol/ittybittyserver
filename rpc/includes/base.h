/* Copyright and license information is at the end of the file */

#ifndef XMLRPC_H_INCLUDED
#define XMLRPC_H_INCLUDED

#include <stddef.h>
#include <stdarg.h>
#include <time.h>
#include "util.h"
#include "config.h"  /* Defines XMLRPC_HAVE_WCHAR */

#if XMLRPC_HAVE_WCHAR
#include <wchar.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif


/*=========================================================================
**  Typedefs
**=========================================================================
**  We define names for these types, because they may change from platform
**  to platform.
*/

typedef signed int xmlrpc_int;  
    /* An integer of the type defined by XML-RPC <int>; i.e. 32 bit */
typedef signed int xmlrpc_int32;
    /* An integer of the type defined by XML-RPC <int4>; i.e. 32 bit */
typedef int xmlrpc_bool;
    /* A boolean (of the type defined by XML-RPC <boolean>, but there's
       really only one kind)
    */
typedef double xmlrpc_double;
    /* A double precision floating point number as defined by
       XML-RPC <float>.  But the C "double" type is universally the same,
       so it's probably clearer just to use that.  This typedef is here 
       for mathematical completeness.
    */

#ifdef _WIN32
typedef SOCKET xmlrpc_socket;
#else
typedef int xmlrpc_socket;
#endif

#define XMLRPC_INT32_MAX (2147483647)
#define XMLRPC_INT32_MIN (-XMLRPC_INT32_MAX - 1)



/*=========================================================================
**  xmlrpc_value
**=========================================================================
**  An XML-RPC value (of any type).
*/

typedef enum {
    XMLRPC_TYPE_INT      = 0,
    XMLRPC_TYPE_BOOL     = 1,
    XMLRPC_TYPE_DOUBLE   = 2,
    XMLRPC_TYPE_DATETIME = 3,
    XMLRPC_TYPE_STRING   = 4,
    XMLRPC_TYPE_BASE64   = 5,
    XMLRPC_TYPE_ARRAY    = 6,
    XMLRPC_TYPE_STRUCT   = 7,
    XMLRPC_TYPE_C_PTR    = 8,
    XMLRPC_TYPE_NIL      = 9,
    XMLRPC_TYPE_DEAD     = 0xDEAD
} xmlrpc_type;

/* These are *always* allocated on the heap. No exceptions. */
typedef struct _xmlrpc_value xmlrpc_value;

void
xmlrpc_abort_if_array_bad(xmlrpc_value * const arrayP);

#define XMLRPC_ASSERT_ARRAY_OK(val) \
    xmlrpc_abort_if_array_bad(val)

/* Increment the reference count of an xmlrpc_value. */
extern void xmlrpc_INCREF (xmlrpc_value* value);

/* Decrement the reference count of an xmlrpc_value. If there
** are no more references, free it. */
extern void xmlrpc_DECREF (xmlrpc_value* value);

/* Get the type of an XML-RPC value. */
extern xmlrpc_type xmlrpc_value_type (xmlrpc_value* value);

xmlrpc_value *
xmlrpc_int_new(xmlrpc_env * const envP,
               int          const intValue);

void 
xmlrpc_read_int(xmlrpc_env *         const envP,
                const xmlrpc_value * const valueP,
                int *                const intValueP);

xmlrpc_value *
xmlrpc_bool_new(xmlrpc_env * const envP,
                xmlrpc_bool  const boolValue);

void
xmlrpc_read_bool(xmlrpc_env *         const envP,
                 const xmlrpc_value * const valueP,
                 xmlrpc_bool *        const boolValueP);

xmlrpc_value *
xmlrpc_double_new(xmlrpc_env * const envP,
                  double       const doubleValue);

void
xmlrpc_read_double(xmlrpc_env *         const envP,
                   const xmlrpc_value * const valueP,
                   xmlrpc_double *      const doubleValueP);

xmlrpc_value *
xmlrpc_datetime_new_str(xmlrpc_env * const envP,
                        const char * const value);

xmlrpc_value *
xmlrpc_datetime_new_sec(xmlrpc_env * const envP, 
                        time_t       const value);

void
xmlrpc_read_datetime_str(xmlrpc_env *         const envP,
                         const xmlrpc_value * const valueP,
                         const char **        const stringValueP);

void
xmlrpc_read_datetime_sec(xmlrpc_env *         const envP,
                         const xmlrpc_value * const valueP,
                         time_t *             const timeValueP);

xmlrpc_value *
xmlrpc_string_new(xmlrpc_env * const envP,
                  const char * const stringValue);

xmlrpc_value *
xmlrpc_string_new_lp(xmlrpc_env * const envP, 
                     size_t       const length,
                     const char * const stringValue);

void
xmlrpc_read_string(xmlrpc_env *         const envP,
                   const xmlrpc_value * const valueP,
                   const char **        const stringValueP);


void
xmlrpc_read_string_lp(xmlrpc_env *         const envP,
                      const xmlrpc_value * const valueP,
                      size_t *             const lengthP,
                      const char **        const stringValueP);

#if XMLRPC_HAVE_WCHAR
xmlrpc_value *
xmlrpc_string_w_new(xmlrpc_env *    const envP,
                    const wchar_t * const stringValue);

xmlrpc_value *
xmlrpc_string_w_new_lp(xmlrpc_env *    const envP, 
                       size_t          const length,
                       const wchar_t * const stringValue);

void
xmlrpc_read_string_w(xmlrpc_env *     const envP,
                     xmlrpc_value *   const valueP,
                     const wchar_t ** const stringValueP);

void
xmlrpc_read_string_w_lp(xmlrpc_env *     const envP,
                        xmlrpc_value *   const valueP,
                        size_t *         const lengthP,
                        const wchar_t ** const stringValueP);

#endif /* XMLRPC_HAVE_WCHAR */

xmlrpc_value *
xmlrpc_base64_new(xmlrpc_env *          const envP, 
                  size_t                const length,
                  const unsigned char * const value);

void
xmlrpc_read_base64(xmlrpc_env *           const envP,
                   const xmlrpc_value *   const valueP,
                   size_t *               const lengthP,
                   const unsigned char ** const bytestringValueP);

void
xmlrpc_read_base64_size(xmlrpc_env *           const envP,
                        const xmlrpc_value *   const valueP,
                        size_t *               const lengthP);

xmlrpc_value *
xmlrpc_array_new(xmlrpc_env * const envP);

/* Return the number of elements in an XML-RPC array.
** Sets XMLRPC_TYPE_ERROR if 'array' is not an array. */
int 
xmlrpc_array_size(xmlrpc_env *         const env, 
                  const xmlrpc_value * const array);

/* Append an item to an XML-RPC array.
** Sets XMLRPC_TYPE_ERROR if 'array' is not an array. */
extern void
xmlrpc_array_append_item (xmlrpc_env   * envP,
                          xmlrpc_value * arrayP,
                          xmlrpc_value * valueP);

void
xmlrpc_array_read_item(xmlrpc_env *         const envP,
                       const xmlrpc_value * const arrayP,
                       unsigned int         const index,
                       xmlrpc_value **      const valuePP);

/* Deprecated.  Use xmlrpc_array_read_item() instead.

   Get an item from an XML-RPC array.
   Does not increment the reference count of the returned value.
   Sets XMLRPC_TYPE_ERROR if 'array' is not an array.
   Sets XMLRPC_INDEX_ERROR if 'index' is out of bounds.
*/
xmlrpc_value * 
xmlrpc_array_get_item(xmlrpc_env *         const envP,
                      const xmlrpc_value * const arrayP,
                      int                  const index);

/* Not implemented--we don't need it yet.
extern 
int xmlrpc_array_set_item (xmlrpc_env* env,
xmlrpc_value* array,
int index,
                                  xmlrpc_value* value);
*/

void
xmlrpc_read_nil(xmlrpc_env *   const envP,
                xmlrpc_value * const valueP);
                

void
xmlrpc_read_cptr(xmlrpc_env *         const envP,
                 const xmlrpc_value * const valueP,
                 void **              const ptrValueP);

xmlrpc_value *
xmlrpc_struct_new(xmlrpc_env * env);

/* Return the number of key/value pairs in a struct.
** Sets XMLRPC_TYPE_ERROR if 'strct' is not a struct. */
int
xmlrpc_struct_size (xmlrpc_env   * env, 
                    xmlrpc_value * strct);

/* Returns true iff 'strct' contains 'key'.
** Sets XMLRPC_TYPE_ERROR if 'strct' is not a struct. */
int 
xmlrpc_struct_has_key(xmlrpc_env *   const envP,
                      xmlrpc_value * const strctP,
                      const char *   const key);

/* The same as the above, but the key may contain zero bytes.
   Deprecated.  xmlrpc_struct_get_value_v() is more general, and this
   case is not common enough to warrant a shortcut.
*/
int 
xmlrpc_struct_has_key_n(xmlrpc_env   * const envP,
                        xmlrpc_value * const strctP,
                        const char *   const key, 
                        size_t         const key_len);

#if 0
/* Not implemented yet, but needed for completeness. */
int
xmlrpc_struct_has_key_v(xmlrpc_env *   env, 
                        xmlrpc_value * strct,
                        xmlrpc_value * const keyval);
#endif


void
xmlrpc_struct_find_value(xmlrpc_env *    const envP,
                         xmlrpc_value *  const structP,
                         const char *    const key,
                         xmlrpc_value ** const valuePP);


void
xmlrpc_struct_find_value_v(xmlrpc_env *    const envP,
                           xmlrpc_value *  const structP,
                           xmlrpc_value *  const keyP,
                           xmlrpc_value ** const valuePP);

void
xmlrpc_struct_read_value_v(xmlrpc_env *    const envP,
                           xmlrpc_value *  const structP,
                           xmlrpc_value *  const keyP,
                           xmlrpc_value ** const valuePP);

void
xmlrpc_struct_read_value(xmlrpc_env *    const envP,
                         xmlrpc_value *  const strctP,
                         const char *    const key,
                         xmlrpc_value ** const valuePP);

/* The "get_value" functions are deprecated.  Use the "find_value"
   and "read_value" functions instead.
*/
xmlrpc_value * 
xmlrpc_struct_get_value(xmlrpc_env *   const envP,
                        xmlrpc_value * const strctP,
                        const char *   const key);

/* The same as above, but the key may contain zero bytes. 
   Deprecated.  xmlrpc_struct_get_value_v() is more general, and this
   case is not common enough to warrant a shortcut.
*/
xmlrpc_value * 
xmlrpc_struct_get_value_n(xmlrpc_env *   const envP,
                          xmlrpc_value * const strctP,
                          const char *   const key, 
                          size_t         const key_len);

/* Set the value associated with 'key' in 'strct' to 'value'.
   Sets XMLRPC_TYPE_ERROR if 'strct' is not a struct. 
*/
void 
xmlrpc_struct_set_value(xmlrpc_env *   const env,
                        xmlrpc_value * const strct,
                        const char *   const key,
                        xmlrpc_value * const value);

/* The same as above, but the key may contain zero bytes.  Deprecated.
   The general way to set a structure value is xmlrpc_struct_set_value_v(),
   and this case is not common enough to deserve a shortcut.
*/
void 
xmlrpc_struct_set_value_n(xmlrpc_env *    const env,
                          xmlrpc_value *  const strct,
                          const char *    const key, 
                          size_t          const key_len,
                          xmlrpc_value *  const value);

/* The same as above, but the key must be an XML-RPC string.
** Fails with XMLRPC_TYPE_ERROR if 'keyval' is not a string. */
void 
xmlrpc_struct_set_value_v(xmlrpc_env *   const env,
                          xmlrpc_value * const strct,
                          xmlrpc_value * const keyval,
                          xmlrpc_value * const value);

/* Given a zero-based index, return the matching key and value. This
** is normally used in conjunction with xmlrpc_struct_size.
** Fails with XMLRPC_TYPE_ERROR if 'struct' is not a struct.
** Fails with XMLRPC_INDEX_ERROR if 'index' is out of bounds. */

void 
xmlrpc_struct_read_member(xmlrpc_env *    const envP,
                          xmlrpc_value *  const structP,
                          unsigned int    const index,
                          xmlrpc_value ** const keyvalP,
                          xmlrpc_value ** const valueP);

/* The same as above, but does not increment the reference count of the
   two values it returns, and return NULL for both if it fails, and
   takes a signed integer for the index (but fails if it is negative).

   Deprecated.  Use xmlrpc_struct_read_member() instead.
*/
void
xmlrpc_struct_get_key_and_value(xmlrpc_env *    env,
                                xmlrpc_value *  strct,
                                int             index,
                                xmlrpc_value ** out_keyval,
                                xmlrpc_value ** out_value);

xmlrpc_value *
xmlrpc_cptr_new(xmlrpc_env * const envP,
                void *       const value);

xmlrpc_value *
xmlrpc_nil_new(xmlrpc_env * const envP);


/* Build an xmlrpc_value from a format string. */

xmlrpc_value * 
xmlrpc_build_value(xmlrpc_env * const env,
                   const char * const format, 
                   ...);

/* The same as the above, but using a va_list and more general */
void
xmlrpc_build_value_va(xmlrpc_env *    const env,
                      const char *    const format,
                      va_list               args,
                      xmlrpc_value ** const valPP,
                      const char **   const tailP);

void 
xmlrpc_decompose_value(xmlrpc_env *   const envP,
                       xmlrpc_value * const value,
                       const char *   const format, 
                       ...);

void 
xmlrpc_decompose_value_va(xmlrpc_env *   const envP,
                          xmlrpc_value * const value,
                          const char *   const format,
                          va_list              args);

/* xmlrpc_parse_value... is the same as xmlrpc_decompose_value... except
   that it doesn't do proper memory management -- it returns xmlrpc_value's
   without incrementing the reference count and returns pointers to data
   inside an xmlrpc_value structure.

   These are deprecated.  Use xmlrpc_decompose_value... instead.
*/
void 
xmlrpc_parse_value(xmlrpc_env *   const envP,
                   xmlrpc_value * const value,
                   const char *   const format, 
                   ...);

/* The same as the above, but using a va_list. */
void 
xmlrpc_parse_value_va(xmlrpc_env *   const envP,
                      xmlrpc_value * const value,
                      const char *   const format,
                      va_list              args);

/*=========================================================================
**  Encoding XML
**=======================================================================*/

/* Serialize an XML value without any XML header. This is primarily used
** for testing purposes. */
void
xmlrpc_serialize_value(xmlrpc_env *       env,
                       xmlrpc_mem_block * output,
                       xmlrpc_value *     value);

/* Serialize a list of parameters without any XML header. This is
** primarily used for testing purposes. */
void
xmlrpc_serialize_params(xmlrpc_env *       env,
                        xmlrpc_mem_block * output,
                        xmlrpc_value *     param_array);

/* Serialize an XML-RPC call. */
void 
xmlrpc_serialize_call (xmlrpc_env *       const env,
                       xmlrpc_mem_block * const output,
                       const char *       const method_name,
                       xmlrpc_value *     const param_array);

/* Serialize an XML-RPC return value. */
extern void
xmlrpc_serialize_response(xmlrpc_env *       env,
                          xmlrpc_mem_block * output,
                          xmlrpc_value *     value);

/* Serialize an XML-RPC fault (as specified by 'fault'). */
extern void
xmlrpc_serialize_fault(xmlrpc_env *       env,
                       xmlrpc_mem_block * output,
                       xmlrpc_env *       fault);


/*=========================================================================
**  Decoding XML
**=======================================================================*/

/* Parse an XML-RPC call. If an error occurs, set a fault and set
** the output variables to NULL.
** The caller is responsible for calling free(*out_method_name) and
** xmlrpc_DECREF(*out_param_array). */
void 
xmlrpc_parse_call(xmlrpc_env *    const envP,
                  const char *    const xml_data,
                  size_t          const xml_len,
                  const char **   const out_method_name,
                  xmlrpc_value ** const out_param_array);

void
xmlrpc_parse_response2(xmlrpc_env *    const envP,
                       const char *    const xmlData,
                       size_t          const xmlDataLen,
                       xmlrpc_value ** const resultPP,
                       int *           const faultCodeP,
                       const char **   const faultStringP);


/* xmlrpc_parse_response() is for backward compatibility */

xmlrpc_value *
xmlrpc_parse_response(xmlrpc_env * const envP, 
                      const char * const xmlData, 
                      size_t       const xmlDataLen);


/*=========================================================================
**  XML-RPC Base64 Utilities
**=========================================================================
**  Here are some lightweight utilities which can be used to encode and
**  decode Base64 data. These are exported mainly for testing purposes.
*/

/* This routine inserts newlines every 76 characters, as required by the
** Base64 specification. */
xmlrpc_mem_block *
xmlrpc_base64_encode(xmlrpc_env *    env,
                     unsigned char * bin_data,
                     size_t          bin_len);

/* This routine encodes everything in one line. This is needed for HTTP
** authentication and similar tasks. */
xmlrpc_mem_block *
xmlrpc_base64_encode_without_newlines(xmlrpc_env *    env,
                                      unsigned char * bin_data,
                                      size_t          bin_len);

/* This decodes Base64 data with or without newlines. */
extern xmlrpc_mem_block *
xmlrpc_base64_decode(xmlrpc_env * const envP,
                     const char * const ascii_data,
                     size_t       const ascii_len);


/*=========================================================================
**  UTF-8 Encoding and Decoding
**=========================================================================
**  We need a correct, reliable and secure UTF-8 decoder. This decoder
**  raises a fault if it encounters invalid UTF-8.
**
**  Note that ANSI C does not precisely define the representation used
**  by wchar_t--it may be UCS-2, UTF-16, UCS-4, or something from outer
**  space. If your platform does something especially bizarre, you may
**  need to reimplement these routines.
*/

/* Ensure that a string contains valid, legally-encoded UTF-8 data.
   (Incorrectly-encoded UTF-8 strings are often used to bypass security
   checks.)
*/
void 
xmlrpc_validate_utf8 (xmlrpc_env * const env,
                      const char * const utf8_data,
                      size_t       const utf8_len);

/* Decode a UTF-8 string. */
xmlrpc_mem_block *
xmlrpc_utf8_to_wcs(xmlrpc_env * const envP,
                   const char * const utf8_data,
                   size_t       const utf8_len);

/* Encode a UTF-8 string. */

#if XMLRPC_HAVE_WCHAR
xmlrpc_mem_block *
xmlrpc_wcs_to_utf8(xmlrpc_env * env,
                   wchar_t *    wcs_data,
                   size_t       wcs_len);
#endif

/*=========================================================================
**  Authorization Cookie Handling
**=========================================================================
**  Routines to get and set values for authorizing via authorization
**  cookies. Both the client and server use HTTP_COOKIE_AUTH to store
**  the representation of the authorization value, which is actually
**  just a base64 hash of username:password. (This entire method is
**  a cookie replacement of basic authentication.)
**/

extern void xmlrpc_authcookie_set(xmlrpc_env * env,
                                  const char * username,
                                  const char * password);

char *xmlrpc_authcookie(void);

#ifdef __cplusplus
}
#endif

/* Copyright (C) 2001 by First Peer, Inc. All rights reserved.
**
** Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions
** are met:
** 1. Redistributions of source code must retain the above copyright
**    notice, this list of conditions and the following disclaimer.
** 2. Redistributions in binary form must reproduce the above copyright
**    notice, this list of conditions and the following disclaimer in the
**    documentation and/or other materials provided with the distribution.
** 3. The name of the author may not be used to endorse or promote products
**    derived from this software without specific prior written permission. 
**  
** THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
** ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
** IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
** ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
** FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
** DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
** OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
** HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
** LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
** OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
** SUCH DAMAGE. */

#endif

