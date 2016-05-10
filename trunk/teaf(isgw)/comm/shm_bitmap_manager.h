/******************************************************************************
* Tencent is pleased to support the open source community by making Teaf available.
* Copyright (C) 2016 THL A29 Limited, a Tencent company. All rights reserved. Licensed under the MIT License (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at http://opensource.org/licenses/MIT
* Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and limitations under the License.
******************************************************************************/
/************************************************************
  Copyright (C), 2008-2018
  FileName: shm_bitmap_manager.h
  Author: anselyang              Date: 2008-12-31
  Description:     申请共享内存，并把国享内存当成BITMAP
                    
                   
  Version:         inicialation
  Function List:   

  History:        
      <author>     <time>     <version >   <desc>
      anselyang  2008-12-31     1.0        create   
***********************************************************/
#ifndef _SHM_BITMAP_MANAGER_H_
#define _SHM_BITMAP_MANAGER_H_

class Shm_Bitmap_Manager
{
public:
    enum { MAX_UIN = 0x80000000 };
    enum { PASSPORT_SHM_SIZE = (MAX_UIN>>3) };
    enum { PASSPORT_SHM_KEY = 0x00710000 };    
    enum { BITMAP_CHK = 0, BITMAP_SET = 1, BITMAP_CLR = 2 };
    
    ~Shm_Bitmap_Manager(void);
    static Shm_Bitmap_Manager* instance(void);

    int open(key_t key=0, ACE_UINT32 max_uin=0);
    int close(void);

    int get_bit(ACE_UINT32 uin);
    int set_bit(ACE_UINT32 uin);
    int clr_bit(ACE_UINT32 uin);
    int count_bit();

private:
    void * get_share_memory(key_t key, size_t size);
    int init_passport_shm(void);
    int handle_shm_bit_i(void * shm_addr, ACE_UINT32 uin, int flag);
    int count_shm_set_bit_num(void * shm_addr);

private:    
    void *      shm_addr_;       // bit map 共享内存映射地址
    static Shm_Bitmap_Manager* instance_;
    ACE_UINT32 max_uin_;    
};

#endif // ifndef _SHM_BITMAP_MANAGER_H_
