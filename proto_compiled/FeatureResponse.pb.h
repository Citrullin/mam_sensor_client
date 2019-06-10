/* Automatically generated nanopb header */
/* Generated by nanopb-0.4.0-dev */

#ifndef PB_ENVIRONMENTSENSORS_FEATURERESPONSE_PB_H_INCLUDED
#define PB_ENVIRONMENTSENSORS_FEATURERESPONSE_PB_H_INCLUDED
#include "pb.h"

/* @@protoc_insertion_point(includes) */
#if PB_PROTO_HEADER_VERSION != 40
#error Regenerate this file with the current version of nanopb generator.
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* Struct definitions */
typedef struct _environmentSensors_FeatureResponse {
    bool hasTemperature;
    bool hasHumidity;
    bool hasAtmosphericPressure;
    bool hasPm2_5;
/* @@protoc_insertion_point(struct:environmentSensors_FeatureResponse) */
} environmentSensors_FeatureResponse;


/* Initializer values for message structs */
#define environmentSensors_FeatureResponse_init_default {0, 0, 0, 0}
#define environmentSensors_FeatureResponse_init_zero {0, 0, 0, 0}

/* Field tags (for use in manual encoding/decoding) */
#define environmentSensors_FeatureResponse_hasTemperature_tag 2
#define environmentSensors_FeatureResponse_hasHumidity_tag 3
#define environmentSensors_FeatureResponse_hasAtmosphericPressure_tag 4
#define environmentSensors_FeatureResponse_hasPm2_5_tag 5

/* Struct field encoding specification for nanopb */
#define environmentSensors_FeatureResponse_FIELDLIST(X, a) \
X(a, STATIC, REQUIRED, BOOL, hasTemperature, 2) \
X(a, STATIC, REQUIRED, BOOL, hasHumidity, 3) \
X(a, STATIC, REQUIRED, BOOL, hasAtmosphericPressure, 4) \
X(a, STATIC, REQUIRED, BOOL, hasPm2_5, 5)
#define environmentSensors_FeatureResponse_CALLBACK NULL
#define environmentSensors_FeatureResponse_DEFAULT NULL

extern const pb_msgdesc_t environmentSensors_FeatureResponse_msg;

/* Defines for backwards compatibility with code written before nanopb-0.4.0 */
#define environmentSensors_FeatureResponse_fields &environmentSensors_FeatureResponse_msg

/* Maximum encoded size of messages (where known) */
#define environmentSensors_FeatureResponse_size  8

#ifdef __cplusplus
} /* extern "C" */
#endif
/* @@protoc_insertion_point(eof) */

#endif
