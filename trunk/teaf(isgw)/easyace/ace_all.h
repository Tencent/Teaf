/******************************************************************************
* Tencent is pleased to support the open source community by making Teaf available.
* Copyright (C) 2016 THL A29 Limited, a Tencent company. All rights reserved. Licensed under the MIT License (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at http://opensource.org/licenses/MIT
* Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and limitations under the License.
******************************************************************************/
#ifndef _ACE_ALL_H_
#define _ACE_ALL_H_

#include "ace/LOCK_SOCK_Acceptor.h"
#include "ace/Auto_Ptr.h"
#include "ace/Reactor.h"
#include "ace/Thread_Manager.h"
#include "ace/Synch.h"
#include "ace/Get_Opt.h"
#include "ace/Logging_Strategy.h"
#include "ace/Log_Msg.h"
#include "ace/Service_Object.h"
#include "ace/svc_export.h"
#include "ace/SOCK_Acceptor.h"
#include "ace/SOCK_Connector.h"
#include "ace/Date_Time.h"
#include "ace/Time_Value.h"
#include "ace/Atomic_Op_T.h"
#include "ace/Configuration.h"
#include "ace/Configuration_Import_Export.h"
#include "ace/SOCK_Connector.h"
#include "ace/SOCK_Dgram.h"
#include "ace/Mem_Map.h"
#include "ace/Svc_Handler.h"
#include "ace/Acceptor.h"
#include "ace/Connector.h"
#include "ace/TP_Reactor.h"
#include "ace/Singleton.h"
#include "ace/Service_Config.h"
#include "ace/ARGV.h"
#include "ace/Reactor_Notification_Strategy.h"
#include "ace/OS_NS_sys_msg.h"
#include "ace/OS_NS_arpa_inet.h"
//#include "ace/OS.h"
#include "ace/Basic_Types.h"
#include "ace/Version.h"
#include "ace/OS_NS_dlfcn.h"
#include "ace/DLL.h"
#include "ace/DLL_Manager.h"

#if ((ACE_MAJOR_VERSION > 5 || (ACE_MAJOR_VERSION==5 && ACE_MINOR_VERSION>=6)) && defined (ACE_HAS_EVENT_POLL) )
#include "ace/Dev_Poll_Reactor.h"
#endif
#if ((ACE_MAJOR_VERSION > 5 || (ACE_MAJOR_VERSION==5 && ACE_MINOR_VERSION>=6)) && defined (ACE_HAS_EVENT_POLL) )
typedef ACE_Reactor_Token_T<ACE_Noop_Token> ACE_Select_Reactor_Noop_Token;
typedef ACE_Select_Reactor_T<ACE_Select_Reactor_Noop_Token> ACE_Select_Reactor_N;
#else
typedef ACE_Select_Reactor_Token_T<ACE_Noop_Token> ACE_Select_Reactor_Noop_Token;
typedef ACE_Select_Reactor_T<ACE_Select_Reactor_Noop_Token> ACE_Select_Reactor_N;
#endif

#endif // _ACE_ALL_H_ 
