/*
 * Copyright 2020 Amazon.com, Inc. or its affiliates. All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License"). You may not use this file except in compliance
 * with the License. A copy of the License is located at
 *
 * http://aws.amazon.com/apache2.0/
 *
 * or in the "license" file accompanying this file. This file is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES
 * OR CONDITIONS OF ANY KIND, either express or implied. See the License for the specific language governing permissions
 * and limitations under the License.
 */
#ifndef DJL_TORCH_DJL_PYTORCH_JNI_UTILS_H
#define DJL_TORCH_DJL_PYTORCH_JNI_UTILS_H

#include <c10/util/typeid.h>
#include <c10/util/variant.h>
#include <torch/csrc/api/include/torch/enum.h>
#include <jni.h>
#include <torch/script.h>

#include <iostream>

#include "djl_pytorch_jni_log.h"

// The file is utilities that are used for JNI

namespace utils {

static constexpr const char* const POINTER_CLASS = "ai/djl/pytorch/jni/Pointer";

static constexpr const jint RELEASE_MODE = JNI_ABORT;

#if !defined(__ANDROID__)
// for image interpolation
typedef torch::variant<
  torch::enumtype::kNearest,
  torch::enumtype::kLinear,
  torch::enumtype::kBilinear,
  torch::enumtype::kBicubic,
  torch::enumtype::kTrilinear,
  torch::enumtype::kArea> mode_t;
#endif

inline jint GetDTypeFromScalarType(const torch::ScalarType& type) {
  if (torch::kFloat32 == type) {
    return 0;
  } else if (torch::kFloat64 == type) {
    return 1;
  } else if (torch::kFloat16 == type) {
    return 2;
  } else if (torch::kUInt8 == type) {
    return 3;
  } else if (torch::kInt32 == type) {
    return 4;
  } else if (torch::kInt8 == type) {
    return 5;
  } else if (torch::kInt64 == type) {
    return 6;
  } else if (torch::kBool == type) {
    return 7;
  } else {
    return 8;
  }
}

inline torch::ScalarType GetScalarTypeFromDType(jint dtype) {
  switch (dtype) {
    case 0:
      return torch::kFloat32;
    case 1:
      return torch::kFloat64;
    case 2:
      return torch::kFloat16;
    case 3:
      return torch::kUInt8;
    case 4:
      return torch::kInt32;
    case 5:
      return torch::kInt8;
    case 6:
      return torch::kInt64;
    case 7:
      return torch::kBool;
    default:
      // TODO improve the error handling
      throw;
  }
}

template <typename T>
inline T* GetPointerFromJHandle(JNIEnv* env, jobject jhandle) {
  jclass jexception = env->FindClass("java/lang/NullPointerException");
  jclass cls = env->FindClass(POINTER_CLASS);
  jmethodID get_value = env->GetMethodID(cls, "getValue", "()J");
  if (get_value == nullptr) {
    env->ThrowNew(jexception, "getValue method not found!");
  }
  jlong ptr = env->CallLongMethod(jhandle, get_value);
  return reinterpret_cast<T*>(ptr);
}

template <typename T>
inline std::vector<T> GetObjectVecFromJHandles(JNIEnv* env, jobjectArray jhandles) {
  jclass jexception = env->FindClass("java/lang/NullPointerException");
  jclass cls = env->FindClass(POINTER_CLASS);
  jmethodID get_value = env->GetMethodID(cls, "getValue", "()J");
  jsize length = env->GetArrayLength(jhandles);
  std::vector<T> vec;
  vec.reserve(length);
  for (auto i = 0; i < length; ++i) {
    jobject jhandle = env->GetObjectArrayElement(jhandles, i);
    if (jhandle == nullptr) {
      env->ThrowNew(jexception, "Pointer not found!");
    }
    jlong ptr = env->CallLongMethod(jhandle, get_value);
    vec.emplace_back(*(reinterpret_cast<T*>(ptr)));
  }
  env->DeleteLocalRef(jexception);
  env->DeleteLocalRef(cls);
  env->DeleteLocalRef(jhandles);
  return std::move(vec);
}

template <typename T>
inline jobject CreatePointer(JNIEnv* env, const T* ptr) {
  jclass jexception = env->FindClass("java/lang/NullPointerException");
  jclass cls = env->FindClass(POINTER_CLASS);
  if (cls == nullptr) {
    env->ThrowNew(jexception, "Pointer class not found!");
  }
  jmethodID init = env->GetMethodID(cls, "<init>", "(J)V");
  jobject new_obj = env->NewObject(cls, init, ptr);
  if (new_obj == nullptr) {
    env->ThrowNew(jexception, "object created failed");
  }
  env->DeleteLocalRef(jexception);
  env->DeleteLocalRef(cls);
  return new_obj;
}

inline std::vector<int64_t> GetVecFromJLongArray(JNIEnv* env, jlongArray jarray) {
  jlong* jarr = env->GetLongArrayElements(jarray, JNI_FALSE);
  jsize length = env->GetArrayLength(jarray);
  const std::vector<int64_t> vec(jarr, jarr + length);
  env->ReleaseLongArrayElements(jarray, jarr, RELEASE_MODE);
  return vec;
}

inline std::vector<int32_t> GetVecFromJIntArray(JNIEnv* env, jintArray jarray) {
  jint* jarr = env->GetIntArrayElements(jarray, JNI_FALSE);
  jsize length = env->GetArrayLength(jarray);
  const std::vector<int32_t> vec(jarr, jarr + length);
  env->ReleaseIntArrayElements(jarray, jarr, RELEASE_MODE);
  return vec;
}

inline std::vector<float> GetVecFromJFloatArray(JNIEnv* env, jfloatArray jarray) {
  jfloat* jarr = env->GetFloatArrayElements(jarray, JNI_FALSE);
  jsize length = env->GetArrayLength(jarray);
  const std::vector<float> vec(jarr, jarr + length);
  env->ReleaseFloatArrayElements(jarray, jarr, RELEASE_MODE);
  return vec;
}

inline torch::Device GetDeviceFromJDevice(JNIEnv* env, jintArray jdevice) {
  jint* device = env->GetIntArrayElements(jdevice, JNI_FALSE);
  auto device_type = static_cast<torch::DeviceType>(*device);
  int device_idx = *(device + 1);
  if (device_type == torch::DeviceType::CPU) {
    device_idx = -1;
  }
  torch::Device torch_device(device_type, device_idx);
  env->ReleaseIntArrayElements(jdevice, device, RELEASE_MODE);
  return torch_device;
}

#if !defined(__ANDROID__)
inline mode_t GetInterpolationMode(jint jmode) {
  switch (jmode) {
    case 0: return torch::kNearest;
    case 1: return torch::kLinear;
    case 2: return torch::kBilinear;
    case 3: return torch::kBicubic;
    case 4: return torch::kTrilinear;
    case 5: return torch::kArea;
    default:
      throw;
  }
}
#endif

inline std::vector<torch::indexing::TensorIndex> CreateTensorIndex(JNIEnv* env, jlongArray jmin_indices, jlongArray jmax_indices, jlongArray jstep_indices) {
  const auto min_indices = GetVecFromJLongArray(env, jmin_indices);
  const auto max_indices = GetVecFromJLongArray(env, jmax_indices);
  const auto step_indices = GetVecFromJLongArray(env, jstep_indices);
  std::vector<torch::indexing::TensorIndex> indices;
  indices.reserve(min_indices.size());
  for (size_t i = 0; i < min_indices.size(); ++i) {
    indices.emplace_back(
      torch::indexing::TensorIndex(torch::indexing::Slice(min_indices[i], max_indices[i], step_indices[i])));
  }
  return indices;
}

inline torch::TensorOptions CreateTensorOptions(
    JNIEnv* env, jint jdtype, jint jlayout, jintArray jdevice, jboolean jrequired_grad) {
  // it gets the device and collect jdevice memory
  const auto device = utils::GetDeviceFromJDevice(env, jdevice);
  auto options = torch::TensorOptions()
                      // for tensor creation API, MKLDNN layout is not supported
                      // the workaround is to create with Strided then call to_mkldnn()
                     .layout((jlayout != 1) ? torch::kStrided : torch::kSparse)
                     .device(device)
                     .requires_grad(JNI_TRUE == jrequired_grad);
  // DJL's UNKNOWN type
  if (jdtype != 8) {
    options = options.dtype(GetScalarTypeFromDType(jdtype));
  }
  return options;
}

inline std::string GetStringFromJString(JNIEnv* env, jstring jstr) {
  if (jstr == nullptr) {
    return std::string();
  }
  const char* c_str = env->GetStringUTFChars(jstr, JNI_FALSE);
  std::string str = std::string(c_str);
  env->ReleaseStringUTFChars(jstr, c_str);
  return str;
}

}  // namespace utils

#endif  // DJL_TORCH_DJL_PYTORCH_JNI_UTILS_H
