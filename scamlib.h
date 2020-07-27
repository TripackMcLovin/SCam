#ifndef SCAMLIB_H
#define SCAMLIB_H

#if defined SCAM_LIB_EXPORT
 #define LIB_EXPORT Q_DECL_EXPORT
#else
 #define LIB_EXPORT Q_DECL_IMPORT
#endif

#endif // SCAMLIB_H
