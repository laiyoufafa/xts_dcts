/*
 * Copyright (C) 2021 Huawei Device Co., Ltd.
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

#include <arpa/inet.h>
#include <cstdio>
#include <dlfcn.h>
#include <gtest/gtest.h>
#include <netdb.h>
#include <pthread.h>
#include <sys/socket.h>

#include "softbus_permission.h"
#include "unistd.h"
#include "net_trans_common.h"
using namespace NetTrans;
using namespace testing::ext;

const int MAX_DATA_LENGTH = 1024;
const char DEF_GROUP_ID[50] = "DEF_GROUP_ID";
const char DEF_PKG_NAME[50] = "com.communication.demo";
const char SESSION_NAME_DATA[50] = "com.communication.demo.data";
const char SHM_DATA_RES[50] = "9999";

const int WAIT_DEF_VALUE = -1;
const int WAIT_SUCCESS_VALUE = 1;
const int WAIT_FAIL_VALUE = 0;
const int SESSION_ID_MIN = 1;
const int MAX_SESSION_NUM = 16;
const int INT_TRUE = 1;
const int FALSE = 0;
const int RET_SUCCESS = 0;

const int STR_PREFIX_FOUR = 4;
const int SLEEP_SECOND_ONE = 1;
const int SLEEP_SECOND_TEO = 2;
const int SLEEP_SECOND_TEN = 10;

const int CODE_LEN_FIVE = 5;
const int CODE_PREFIX_FOUR = 4;

static int32_t g_currentSessionId4Data = -1;
static int32_t g_waitFlag = WAIT_DEF_VALUE;
static int32_t g_waitFlag4Ctl = WAIT_DEF_VALUE;
static int32_t g_waitFlag4Data = WAIT_DEF_VALUE;
static int32_t g_nodeOnlineCount = 0;
static int32_t g_nodeOfflineCount = 0;
static int32_t g_sessionOpenCount4Data = 0;
static SessionAttribute* g_sessionAttr4Data = nullptr;
static ISessionListener* g_sessionlistener4Data = nullptr;
static char g_networkId[NETWORK_ID_BUF_LEN] = { 0 };
static INodeStateCb g_defNodeStateCallback;
static ConnectionAddr g_ethAddr = {
    .type = CONNECTION_ADDR_WLAN,
};

static int g_subscribeId = 0;

namespace NetTrans {
class NetTransCommon : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();
    NetTransCommon();
};
void NetTransCommon::SetUpTestCase(void) {}

void NetTransCommon::TearDownTestCase(void) {}
void NetTransCommon::SetUp(void) {}

void NetTransCommon::TearDown(void) {}
NetTransCommon::NetTransCommon(void) {}

int Wait4Session(int timeout, WaitSessionType type)
{
    int hitFlag = -1;
    while (timeout > 0) {
        sleep(SLEEP_SECOND_ONE);
        switch (type) {
            case WaitSessionType::SESSION_4CTL:
                if (g_waitFlag4Ctl != WAIT_DEF_VALUE) {
                    LOG("Wait4Session success,flag:%d", g_waitFlag4Ctl);
                    hitFlag = 1;
                }
                break;
            case WaitSessionType::SESSION_4DATA:
                if (g_waitFlag4Data != WAIT_DEF_VALUE) {
                    LOG("Wait4Session success,flag:%d", g_waitFlag4Ctl);
                    hitFlag = 1;
                }
                break;
            default:
                LOG("Wait4Session type error");
                hitFlag = 1;
                break;
        }
        if (hitFlag != -1) {
            break;
        }
        timeout--;
    }
    switch (type) {
        case WaitSessionType::SESSION_4CTL:
            if (g_waitFlag4Ctl != WAIT_SUCCESS_VALUE) {
                LOG("Wait4Session FAIL,flag:%d", g_waitFlag4Ctl);
                return SOFTBUS_ERR;
            }
            break;
        case WaitSessionType::SESSION_4DATA:
            if (g_waitFlag4Data != WAIT_SUCCESS_VALUE) {
                LOG("Wait4Session FAIL,flag:%d", g_waitFlag4Ctl);
                return SOFTBUS_ERR;
            }
            break;
        default:
            LOG("Wait4Session type error");
            return SOFTBUS_ERR;
    }
    return SOFTBUS_OK;
}

void OnDefNodeOnline(NodeBasicInfo* info)
{
    LOG("OnDefNodeOnline");
    if (info == nullptr) {
        LOG("OnDefNodeOnline info is nullptr");
        return;
    }
    if (strncpy_s(g_networkId, NETWORK_ID_BUF_LEN, info->networkId, NETWORK_ID_BUF_LEN) != RET_SUCCESS) {
        return;
    }
    LOG("Online id:%s,name:%s,type id:%u", info->networkId, info->deviceName, info->deviceTypeId);
    g_nodeOnlineCount++;
}

void OnDefNodeOffline(NodeBasicInfo* info)
{
    LOG("OnDefNodeOnline");
    if (info == nullptr) {
        LOG("OnDefNodeOnline info is nullptr");
        return;
    }
    LOG("Offline id:%s,name:%s,type id:%u", info->networkId, info->deviceName, info->deviceTypeId);
    g_nodeOfflineCount++;
}

void OnDefNodeBasicInfoChanged(NodeBasicInfoType type, NodeBasicInfo* info)
{
    if (info == nullptr) {
        LOG("OnDefNodeBasicInfoChanged info is nullptr");
        return;
    }

    LOG("InfoChanged id:%s,name:%s", info->networkId, info->deviceName);
}

int DataSessionOpened(int sessionId, int result)
{
    LOG("DataSessionOpened sessionId=%d,result=%d", sessionId, result);
    g_sessionOpenCount4Data++;
    if (result == SOFTBUS_OK) {
        SetCurrentSessionId4Data(sessionId);
        if (sessionId == g_currentSessionId4Data) {
            LOG("openSession check success sessionId=%d", sessionId);
            g_waitFlag4Data = WAIT_SUCCESS_VALUE;
        } else {
            LOG("open session callback %d not match open sid %d", sessionId, g_currentSessionId4Data);
            g_waitFlag4Data = WAIT_FAIL_VALUE;
        }
    } else {
        g_waitFlag4Data = WAIT_FAIL_VALUE;
    }

    return SOFTBUS_OK;
}

void ResetWaitFlag4Data(void)
{
    g_waitFlag = WAIT_DEF_VALUE;
}

void ResetWaitCount4Offline(void)
{
    g_nodeOfflineCount = 0;
}

void ResetWaitCount4Online(void)
{
    g_nodeOnlineCount = 0;
}

void DataSessionClosed(int sessionId)
{
    LOG("close session %d", sessionId);
}

void DataBytesReceived(int sessionId, const void* data, unsigned int dataLen)
{
    LOG("byteRec start");
}

void DataMessageReceived(int sessionId, const void* data, unsigned int dataLen)
{
    LOG("MsgRec start sessionId=%d, dataLen = %d, data=%s", sessionId, dataLen, data);

    unsigned int maxLen = MAX_DATA_LENGTH;
    if (dataLen <= maxLen) {
        int* code = (int*)malloc(sizeof(int));
        char* buf = (char*)malloc(MAX_DATA_LENGTH);
        (void)memset_s(buf, MAX_DATA_LENGTH, 0, MAX_DATA_LENGTH);
        if (strncpy_s(buf, MAX_DATA_LENGTH, (char*)data, dataLen) != RET_SUCCESS) {
            return;
        }
        if (*code != -1) {
            pthread_t thread;
            int ret = pthread_create(&thread, nullptr, DataOperateTask, buf);
            pthread_join(thread, nullptr);
            LOG("create thread ret:%d", ret);
        } else {
            free(code);
        }
    }
}

int RegisterDeviceStateDefCallback(void)
{
    return RegNodeDeviceStateCb(DEF_PKG_NAME, &g_defNodeStateCallback);
}

int UnRegisterDeviceStateDefCallback(void)
{
    return UnregNodeDeviceStateCb(&g_defNodeStateCallback);
}

int CreateSsAndOpenSession4Data()
{
    int ret = CreateSessionServer(DEF_PKG_NAME, SESSION_NAME_DATA, g_sessionlistener4Data);
    if (ret != SOFTBUS_OK) {
        LOG("call createSessionServer fail, ret:%d", ret);
        return ret;
    }

    pthread_t sendThread;
    pthread_create(&sendThread, nullptr, SendMsgTask, nullptr);
    pthread_join(sendThread, nullptr);

    return ret;
}

int OpenSession4Data(void)
{
    int sessionId;

    sessionId = OpenSession(SESSION_NAME_DATA, SESSION_NAME_DATA, g_networkId, DEF_GROUP_ID, g_sessionAttr4Data);
    if (sessionId < SESSION_ID_MIN) {
        LOG("call open session fail ssid:%d", sessionId);
        return SOFTBUS_ERR;
    }

    LOG("call open session SUCCESS ssid:%d", sessionId);
    sleep(SLEEP_SECOND_TEN);
    return 0;
}

int SendDataMsgToRemote(CtrlCodeType code, char* data)
{
    int ret = -1;
    int size = MAX_DATA_LENGTH;
    char* msg = (char*)malloc(size);
    if (msg == nullptr) {
        return ret;
    }
    (void)memset_s(msg, size, 0, size);

    if (strcpy_s(msg, size, data) != RET_SUCCESS) {
        return ret;
    }
    ret = SendMessage(g_currentSessionId4Data, msg, strlen(msg));
    LOG("send msg ret:%d", ret);
    free(data);
    return ret;
}

int CloseSessionAndRemoveSs4Data(void)
{
    int ret;
    int timeout = 10;
    ResetWaitFlag4Data();
    CloseSession(g_currentSessionId4Data);
    ret = Wait4Session(timeout, WaitSessionType::SESSION_4DATA);
    if (ret == SOFTBUS_OK) {
        LOG("close session success");
    }

    int retss = RemoveSessionServer(DEF_PKG_NAME, SESSION_NAME_DATA);
    if (retss != SOFTBUS_OK) {
        LOG("remove session ret:%d", retss);
    }

    if (ret != SOFTBUS_OK || retss == SOFTBUS_OK) {
        return SOFTBUS_ERR;
    } else {
        return SOFTBUS_OK;
    }
}

int IncrementSubId(void)
{
    g_subscribeId++;
    return g_subscribeId;
}

void OnDataMessageReceived(int sessionId, const char* data, unsigned int dataLen)
{
    LOG("msg received %s", data);
    if (sessionId < 0 || sessionId > MAX_SESSION_NUM) {
        LOG("message received invalid session %d", sessionId);
        return;
    }

    LOG("msg received sid:%d, data-len:%d", sessionId, dataLen);

    unsigned int maxLen = MAX_DATA_LENGTH;
    if (dataLen <= maxLen) {
        int* code = (int*)malloc(sizeof(int));
        char* buf = (char*)malloc(MAX_DATA_LENGTH);
        if (strcpy_s(buf, MAX_DATA_LENGTH, data) != RET_SUCCESS) {
            return;
        }
        if (*code != -1) {
            pthread_t thread;
            int ret = pthread_create(&thread, nullptr, DataOperateTask, buf);
            pthread_join(thread, nullptr);
            LOG("create thread ret:%d", ret);
        } else {
            free(code);
        }
    }
}

void* SendMsgTask(void* param)
{
    LOG("SendMsgTask send...%s", param);
    int sessionId;
    int timeout = 10;
    int ret = 0;
    ResetWaitFlag4Data();

    sessionId = OpenSession(SESSION_NAME_DATA, SESSION_NAME_DATA, g_networkId, DEF_GROUP_ID, g_sessionAttr4Data);
    if (sessionId < SESSION_ID_MIN) {
        LOG("call open session faild ret:%d", sessionId);
    }
    SetCurrentSessionId4Data(sessionId);
    LOG("call open session success sid:%d", sessionId);

    ret = Wait4Session(timeout, WaitSessionType::SESSION_4DATA);
    if (ret != SOFTBUS_OK) {
        LOG("open session fail");
    }

    if (createShm(SHM_SEND_KEY) == -1) {
        LOG("create shm faild");
        return nullptr;
    }

    initShm();

    char str[MAX_DATA_LENGTH] = { 0 };
    while (INT_TRUE) {
        if (readDataFromShm(str) == 0) {
            if (strncmp(SHM_DATA_RES, str, STR_PREFIX_FOUR) == 0) {
                LOG("read result");
            } else {
                LOG("SendData send...%s", str);
                SendMessage(g_currentSessionId4Data, str, strlen(str));
                (void)memset_s(str, strlen(str) + 1, 0, strlen(str) + 1);
            }
        }
        sleep(SLEEP_SECOND_TEO);
    }

    LOG("sendMsgTask end");
    return nullptr;
}

void* DataOperateTask(void* param)
{
    LOG("operate start...");
    int code = -1;
    char codeType[CODE_LEN_FIVE] = { 0 };
    if (strncpy_s(codeType, CODE_LEN_FIVE, (char*)param, CODE_PREFIX_FOUR) != RET_SUCCESS) {
        return nullptr;
    }
    if (sscanf_s(codeType, "%d", &code) <= RET_SUCCESS) {
        return nullptr;
    }
    LOG("code :%d", code);

    void* handle = nullptr;
    int (*ProcessData)(int, char*);
    char* error;
    int ret = 0;
    if (code == int(CtrlCodeType::CTRL_CODE_RESULT_TYPE)) {
        writeDataToShm((char*)param);
        free((char*)param);
        return nullptr;
    } else if (code > int(CtrlCodeType::CTRL_CODE_DATAMGR_TYPE) && code < int(CtrlCodeType::CTRL_CODE_DM_TYPE)) {
        handle = dlopen("/system/lib64/libdisDataProcess.z.so", RTLD_LAZY);
        if (!handle) {
            LOG("dlopen failed %s", dlerror());
        }
        ProcessData = (int (*)(int, char*))dlsym(handle, "_Z14ProcessDataMgriPc");
        if ((error = dlerror()) != nullptr) {
            LOG("dlsym failed %s", dlerror());
        }
        ret = (*ProcessData)(code, (char*)param);
        LOG("code:%d", ret);
    }

    char* str = (char*)malloc(MAX_DATA_LENGTH);
    if (str == nullptr) {
        return nullptr;
    }
    (void)memset_s(str, MAX_DATA_LENGTH, 0, MAX_DATA_LENGTH);
    int resSprint = sprintf_s(str, MAX_DATA_LENGTH, "%d:%d", CtrlCodeType::CTRL_CODE_RESULT_TYPE, ret);
    if (resSprint < FALSE) {
        return nullptr;
    }
    SendDataMsgToRemote(CtrlCodeType::CTRL_CODE_RESULT_TYPE, str);
    if (handle) {
        dlclose(handle);
    }
    free((char*)param);
    LOG("operate end");
    return nullptr;
}

int CheckRemoteDeviceIsNull(int isSetNetId)
{
    int nodeNum = 0;
    NodeBasicInfo* nodeInfo = nullptr;
    int ret = GetAllNodeDeviceInfo(DEF_PKG_NAME, &nodeInfo, &nodeNum);
    LOG("get node number is %d, ret %d", nodeNum, ret);
    if (nodeInfo != nullptr && nodeNum > 0) {
        LOG("get neiId is %s", nodeInfo->networkId);
        if (isSetNetId == INT_TRUE) {
            if (strncpy_s(g_networkId, NETWORK_ID_BUF_LEN, nodeInfo->networkId, NETWORK_ID_BUF_LEN) != RET_SUCCESS) {
                return SOFTBUS_ERR;
            }
        }
        FreeNodeInfo(nodeInfo);
        return SOFTBUS_OK;
    } else {
        LOG("nodeInfo is nullptr");
        return SOFTBUS_ERR;
    }
}

ISessionListener* GetSessionListenser4Data(void)
{
    return g_sessionlistener4Data;
}

void SetCurrentSessionId4Data(int sessionId)
{
    g_currentSessionId4Data = sessionId;
}

int GetCurrentSessionId4Data(void)
{
    return g_currentSessionId4Data;
}

ConnectionAddr* GetConnectAddr(void)
{
    return &g_ethAddr;
}

void init(void)
{
    g_defNodeStateCallback.events = EVENT_NODE_STATE_MASK;
    g_defNodeStateCallback.onNodeOnline = OnDefNodeOnline;
    g_defNodeStateCallback.onNodeOffline = OnDefNodeOffline;
    g_defNodeStateCallback.onNodeBasicInfoChanged = OnDefNodeBasicInfoChanged;

    if (g_sessionlistener4Data == nullptr) {
        g_sessionlistener4Data = (ISessionListener*)calloc(1, sizeof(ISessionListener));
        g_sessionlistener4Data->OnSessionOpened = DataSessionOpened;
        g_sessionlistener4Data->OnSessionClosed = DataSessionClosed;
        g_sessionlistener4Data->OnMessageReceived = DataMessageReceived;
        g_sessionlistener4Data->OnBytesReceived = DataBytesReceived;
    }

    if (g_sessionAttr4Data == nullptr) {
        g_sessionAttr4Data = (SessionAttribute*)calloc(1, sizeof(SessionAttribute));
        g_sessionAttr4Data->dataType = TYPE_BYTES;
    }

    SoftBusPermission::AddPermission(DEF_PKG_NAME);

    int ret = RegisterDeviceStateDefCallback();
    if (ret != SOFTBUS_OK) {
        LOG("RegisterDeviceStateDefCallback FAILED ret=%d", ret);
    }
}

void destroy(void)
{
    if (g_sessionlistener4Data != nullptr) {
        free(g_sessionlistener4Data);
        g_sessionlistener4Data = nullptr;
    }
}
/**
 *  @tc.number: DisTest_0001
 *  @tc.name: net trans common
 *  @tc.desc: net trans common
 *  @tc.type: FUNC
 */
HWTEST_F(NetTransCommon, DisTest_0001, TestSize.Level1)
{
    LOG("enter main");
    init();
    while (INT_TRUE) {
        int ret = CheckRemoteDeviceIsNull(INT_TRUE);
        if (ret == SOFTBUS_OK) {
            break;
        } else {
            sleep(SLEEP_SECOND_TEO);
        }
    }

    int ret = CreateSsAndOpenSession4Data();
    if (ret != SOFTBUS_OK) {
        LOG("CreateSsAndOpenSession4Ctl failed ret=%d", ret);
    }

    while (INT_TRUE) {
        LOG("enter while success");
        sleep(SLEEP_SECOND_TEN);
    }
}
}; // namespace NetTrans