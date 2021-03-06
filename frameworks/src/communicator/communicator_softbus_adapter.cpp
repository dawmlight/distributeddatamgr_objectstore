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

#include "communicator_softbus_adapter.h"

#include <memory>

#include "objectstore_errors.h"
#include "logger.h"
#include "rpc_network.h"

namespace OHOS::ObjectStore {
static const char *SESSION_NAME = "objectstore";
static char const *GROUP_ID = "";

static SessionAttribute g_sessionAttr = {
    .dataType = SessionType::TYPE_BYTES
};

uint32_t CommunicatorSoftbusAdapter::GetDeviceIdBySession(int sessionId, std::string &deviceId)
{
    char devId[256] = {0};
    int errCode = ::GetPeerDeviceId(sessionId, devId, 256);
    if (errCode != 0) {
        return ERR_OPENSESSION;
    }
    deviceId = devId; // for reload "=" after C++11
    return SUCCESS;
}


int CommunicatorSoftbusAdapter::Init(std::shared_ptr<SoftBusListener> &networkListener)
{
    if (networkListener == nullptr) {
        return SUCCESS;
    }
    RegNodeDeviceStateCb(PKG_NAME, networkListener->GetNodeStateCallback());
    LOG_INFO("enter CreateSessionServer");
    int ret = CreateSessionServer(PKG_NAME, SESSION_NAME, networkListener->GetSessionListener());
    if (ret != 0) {
        LOG_INFO("CreateSessionServer failed");
        return ERR_OPENSESSION;
    }
    {
        std::unique_lock<std::mutex> sessionLock(operationMutex_);
        networkListener_ = networkListener;
    
    }
    return SUCCESS;
}

uint32_t CommunicatorSoftbusAdapter::OpenSoftbusLink(const std::string &networkName, const std::string &networkId)
{
    {
        std::unique_lock<std::mutex> sessionLock(operationMutex_);
        if (0 != sessionDevDic_.count(networkId)) {
            if (networkListener_ != nullptr && networkListener_->GetSessionListener() != nullptr
                && networkListener_->GetSessionListener()->OnSessionOpened != nullptr) {
                networkListener_->GetSessionListener()->OnSessionOpened(sessionDevDic_[networkId], SUCCESS);
            }
            LOG_INFO("has opened %s", networkId.c_str());
            return SUCCESS;
        }
    }
    int32_t sessionId;
    LOG_INFO("OpenSession g_networkId = %s", networkId.c_str());
    sessionId = OpenSession(SESSION_NAME, SESSION_NAME, networkId.c_str(), GROUP_ID, &g_sessionAttr);
    if (sessionId < 0) {
        LOG_ERROR("OpenSession failed %d", sessionId);
        if (networkListener_ != nullptr && networkListener_->GetSessionListener() != nullptr
            && networkListener_->GetSessionListener()->OnSessionOpened != nullptr) {
            networkListener_->GetSessionListener()->OnSessionOpened(sessionDevDic_[networkId], ERR_OPENSESSION);
        }
        return ERR_OPENSESSION;
    }
    {
        std::unique_lock<std::mutex> sessionLock(operationMutex_);
        sessionDevDic_[networkId] = sessionId;
    }
    LOG_INFO("OpenSession success");
    return SUCCESS;
}

uint32_t CommunicatorSoftbusAdapter::Destroy()
{
    int ret = RemoveSessionServer(PKG_NAME, SESSION_NAME);
    if (ret != 0) {
        LOG_ERROR("RemoveSessionServer failed");
        return ERR_NETWORK;
    }
    UnregNodeDeviceStateCb(networkListener_->GetNodeStateCallback());
    {
        std::unique_lock<std::mutex> sessionLock(operationMutex_);
        sessionDevDic_.clear();
        networkListener_ = nullptr;
    }
    LOG_INFO("CloseSoftbusLink success");
    return SUCCESS;
}

uint32_t CommunicatorSoftbusAdapter::SendMsg(const std::string &networkId, const uint8_t *data,
        uint32_t dataSize, int32_t type)
{
    int32_t ret = SUCCESS;
    int32_t sessionId = 0;
    {
        std::unique_lock<std::mutex> sessionLock(operationMutex_);
        if (0 == sessionDevDic_.count(networkId)) {
            LOG_INFO("network id not exit %s", networkId.c_str());
            return SUCCESS;
        }
        sessionId = sessionDevDic_[networkId];
    }
    LOG_INFO("start sendMsg %d", dataSize);
    if (type == TYPE_SEND_BYTE) {
        ret = SendBytes(sessionId, data, dataSize);
    }
    if (type == TYPE_SEND_MESSAGE) {
        ret = SendMessage(sessionId, data, dataSize);
    }
    if (ret != 0) {
        return ERR_NETWORK;
    }
    LOG_INFO("sendMsg success");
    return SUCCESS;
}
}  // namespace OHOS::ObjectStore

