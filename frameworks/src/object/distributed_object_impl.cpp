/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <type_traits>

#include "distributed_object_impl.h"
#include "objectstore_errors.h"
#include "string_utils.h"
#include "bytes.h"
#include "logger.h"

namespace OHOS::ObjectStore {
DistributedObjectImpl::DistributedObjectImpl(FlatObject *flatObject) : flatObject_(flatObject) {};
DistributedObjectImpl::~DistributedObjectImpl()
{
    delete flatObject_;
}

namespace {
int32_t VAL_NULL = -1;      // null
int32_t VAL_STRING = 0;     // string
int32_t VAL_INTEGER = 1;    // integer
int32_t VAL_BOOLEAN = 2;    // boolean
int32_t VAL_SHORT = 3;      // short
int32_t VAL_LONG = 4;       // long
int32_t VAL_FLOAT = 5;      // float
int32_t VAL_DOUBLE = 6;     // double
int32_t VAL_CHARACTER = 7;  // character
int32_t VAL_BYTE = 8;  // character
int32_t VAL_OBJECT = 9;  // character

inline bool IsBigEndian()
{
    uint32_t data = 0x12345678; // 0x12345678 only used here, for endian test
    uint8_t *firstByte = reinterpret_cast<uint8_t *>(&data);
    if (*firstByte == 0x12) { // 0x12 only used here, for endian test
        return true;
    }
    return false;
}

template<typename T> T HostToNet(const T &from)
{
    if (IsBigEndian()) {
        return from;
    } else {
        T to;
        size_t typeLen = sizeof(T);
        const uint8_t *fromByte = reinterpret_cast<const uint8_t *>(&from);
        uint8_t *toByte = reinterpret_cast<uint8_t *>(&to);
        for (size_t i = 0; i < typeLen; i++) {
            toByte[i] = fromByte[typeLen - i - 1]; // 1 is for index boundary
        }
        return to;
    }
}

template<typename T> T NetToHost(const T &from)
{
    return HostToNet(from);
}

// Is same as > L2, use Little-Endian, 64-bits system
void PutNum(int32_t offset, void* val, int32_t valLen, Bytes &data)
{
    uint32_t len = valLen + offset;
    if (len > sizeof(data.front()) * data.size()) {
        data.resize(len);
    }

    for (int i = 0; i < valLen; i++) {
        data[offset + i] = *(static_cast<uint64_t *>(val)) >> (valLen - i - 1) * 8;
    }
    return;
}

void TransBytes(void* input, int32_t inputLen, int32_t flag, Bytes &data)
{
    data.resize(sizeof(int32_t) + inputLen, 0);
    PutNum(0, &flag, sizeof(flag), data);
    PutNum(sizeof(int32_t), input, inputLen, data);
    return;
}
}

uint32_t DistributedObjectImpl::PutChar(const std::string &key, char value)
{
    Bytes data;
    uint16_t val = value;
    TransBytes(&val, sizeof(val), VAL_CHARACTER, data);
    flatObject_->SetField(StrToFieldBytes(key), data);
    return SUCCESS;
}

uint32_t DistributedObjectImpl::PutInt(const std::string &key, int32_t value) {
    Bytes data;
    TransBytes(&value, sizeof(value), VAL_INTEGER, data);
    flatObject_->SetField(StrToFieldBytes(key), data);
    return SUCCESS;
}

uint32_t DistributedObjectImpl::PutShort(const std::string &key, int16_t value)
{
    Bytes data;
    TransBytes(&value, sizeof(value), VAL_SHORT, data);
    flatObject_->SetField(StrToFieldBytes(key), data);
    return SUCCESS;
}


Bytes DistributedObjectImpl::StrToFieldBytes(const std::string &src)
{
    Bytes data;
    PutNum(0, &VAL_STRING, sizeof(VAL_STRING), data);
    Bytes dst = StringUtils::StrToBytes(src);
    data.insert(data.end(), dst.begin(), dst.end());
    return data;
}

uint32_t DistributedObjectImpl::PutLong(const std::string &key, int64_t value)
{
    Bytes data;
    TransBytes(&value, sizeof(value), VAL_LONG, data);
    flatObject_->SetField(StrToFieldBytes(key), data);
    return SUCCESS;
}

uint32_t DistributedObjectImpl::PutFloat(const std::string &key, float value)
{
    Bytes data;
    TransBytes(&value, sizeof(value), VAL_FLOAT, data);
    flatObject_->SetField(StrToFieldBytes(key), data);
    return SUCCESS;
}

uint32_t DistributedObjectImpl::PutDouble(const std::string &key, double value)
{
    Bytes data;
    TransBytes(&value, sizeof(value), VAL_DOUBLE, data);
    flatObject_->SetField(StrToFieldBytes(key), data);
    return SUCCESS;
}

uint32_t DistributedObjectImpl::PutBoolean(const std::string &key, bool value)
{
    Bytes data;
    int32_t val = value?1:0;
    TransBytes(&val, sizeof(val), VAL_BOOLEAN, data);
    flatObject_->SetField(StrToFieldBytes(key), data);
    return SUCCESS;
}

uint32_t DistributedObjectImpl::PutString(const std::string &key, const std::string &value)
{
    flatObject_->SetField(StrToFieldBytes(key), StrToFieldBytes(value));
    return SUCCESS;
}

uint32_t DistributedObjectImpl::PutByte(const std::string &key, int8_t value)
{
    Bytes data;
    TransBytes(&value, sizeof(value), VAL_BYTE, data);
    flatObject_->SetField(StrToFieldBytes(key), data);
    return SUCCESS;
}

FlatObject *DistributedObjectImpl::GetObject()
{
    return flatObject_;
}
}