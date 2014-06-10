﻿#include "StdAfx.h"
#include "CommandHandler.h"
#include "PacketDef/ServerPacket.h"
#include "PacketDef/TransferPacket.h"
#include "DataBuffer/BufferHelper.h"
#include "ConnectionType.h"
#include "NetworkMgr.h"
#include "PacketDef/ClientPacket.h"
#include "resource.h"
#include "TestClientDlg.h"
#include "DataBuffer/BufferHelper.h"
#include "PacketDef/LoginPacket.h"

CClientCmdHandler::CClientCmdHandler(void)
{
}

CClientCmdHandler::~CClientCmdHandler(void)
{
}

CClientCmdHandler* CClientCmdHandler::GetInstancePtr()
{
	static CClientCmdHandler _Handler;

	return &_Handler;
}

BOOL CClientCmdHandler::OnCommandHandle( UINT16 wCommandID, UINT64 u64ConnID, CBufferHelper *pBufferHelper )
{
	switch(wCommandID)
	{
		PROCESS_COMMAND_ITEM(CMD_CONNECT_NOTIFY,		OnCmdConnectNotify);

		PROCESS_COMMAND_ITEM(CMD_CHAR_LOGIN_ACK,		OnCmdLoginGameAck);

		PROCESS_COMMAND_ITEM(CMD_CHAR_NEARBY_ADD,		OnCmdNearByAdd);

		PROCESS_COMMAND_ITEM(CMD_CHAR_NEARBY_UPDATE,	OnCmdNearByUpdate);

		PROCESS_COMMAND_ITEM(CMD_CHAR_NEARBY_REMOVE,	OnCmdNearByRemove);

		PROCESS_COMMAND_ITEM(CMD_CHAR_ENTER_GAME_ACK,	OnCmdEnterGameAck);

	default:
		{

		}
		break;
	}

	return TRUE;
}


UINT32 CClientCmdHandler::OnCmdConnectNotify(UINT16 wCommandID, UINT64 u64ConnID, CBufferHelper *pBufferHelper)
{
	StConnectNotify ConnectNotify;

	pBufferHelper->Read(ConnectNotify);

	UINT32 ConType = ConnectNotify.btConType;

	ConnectNotify.btConType =  TYPE_CLT_PLAYER;

	ConnectNotify.u64ConnID = m_HostPlayer.GetObjectID();

	CBufferHelper WriteHelper(TRUE, CNetworkMgr::GetInstancePtr()->m_pWriteBuffer);

	WriteHelper.BeginWrite(CMD_CONNECT_NOTIFY, CMDH_SVR_CON, 0, 0);

	WriteHelper.Write(ConnectNotify);

	WriteHelper.EndWrite();

	CNetworkMgr::GetInstancePtr()->SendData(CNetworkMgr::GetInstancePtr()->m_pWriteBuffer->GetData(), CNetworkMgr::GetInstancePtr()->m_pWriteBuffer->GetDataLenth());


	if(ConType == TYPE_SVR_PROXY)
	{
		StCharEnterGameReq CharEnterGameReq;

		CharEnterGameReq.u64CharID = m_HostPlayer.GetObjectID();

		CBufferHelper WriteHelper(TRUE, CNetworkMgr::GetInstancePtr()->m_pWriteBuffer);

		WriteHelper.BeginWrite(CMD_CHAR_ENTER_GAME_REQ, CMDH_SENCE, 12, CharEnterGameReq.u64CharID);

		WriteHelper.Write(CharEnterGameReq);

		WriteHelper.EndWrite();

		CNetworkMgr::GetInstancePtr()->SendData(CNetworkMgr::GetInstancePtr()->m_pWriteBuffer->GetData(), CNetworkMgr::GetInstancePtr()->m_pWriteBuffer->GetDataLenth());
	}

	return 0;
}

UINT32 CClientCmdHandler::OnCmdLoginGameAck( UINT16 wCommandID, UINT64 u64ConnID, CBufferHelper *pBufferHelper )
{
	StCharLoginAck MsgLoginAck;

	pBufferHelper->Read(MsgLoginAck);

	if(MsgLoginAck.byteCode == 0)
	{
		printf("登录失败!");
	}
	else
	{
		printf("登录成功!");
		CNetworkMgr::GetInstancePtr()->DisConnect();
		m_HostPlayer.SetObjectID(MsgLoginAck.u64CharID);
		CNetworkMgr::GetInstancePtr()->ConnectToServer("127.0.0.1", 7998);
	}
	

	return TRUE;
}

UINT32 CClientCmdHandler::OnCmdNearByAdd( UINT16 wCommandID, UINT64 u64ConnID, CBufferHelper *pBufferHelper )
{
	UINT32 dwCount = 0;
	pBufferHelper->Read(dwCount);

	printf("BEGIN---添加角色消息，添加人数:%d", dwCount);
	

	for(UINT32 i = 0; i < dwCount; i++)
	{
		CString strText;

		UINT8 type = 0;

		pBufferHelper->Read(type);

		if(type != 1)
		{
			continue;
		}

		CPlayerObject *pObject = new CPlayerObject;

		pObject->ReadFromBuffer(pBufferHelper);

		m_PlayerObjMgr.insert(std::make_pair(pObject->GetObjectID(), pObject));

		printf("添加角色:%d, 坐标x = %f, z = %f", (UINT32)pObject->GetObjectID(), pObject->m_ObjectPos.x, pObject->m_ObjectPos.z);

	}

	printf("END---添加角色消息");


	((CTestClientDlg*)AfxGetMainWnd())->m_DlgGame.Invalidate();

	return 0;
}

UINT32 CClientCmdHandler::OnCmdNearByUpdate( UINT16 wCommandID, UINT64 u64ConnID, CBufferHelper *pBufferHelper )
{
	UINT32 dwCount = 0;
	pBufferHelper->Read(dwCount);

	for(UINT32 i = 0; i < dwCount; i++)
	{
		UINT64 u64CharID = 0;

		pBufferHelper->Read(u64CharID);

		CPlayerObject *pObject = m_PlayerObjMgr.GetPlayer(u64CharID);
		if(pObject != NULL)
		{
			pObject->ReadFromBuffer(pBufferHelper);

			printf("更新角色:%d, 坐标x = %f, z = %f", (UINT32)pObject->GetObjectID(), pObject->m_ObjectPos.x, pObject->m_ObjectPos.z);
		}
		else
		{
			ASSERT_FAIELD;
		}
	}

	((CTestClientDlg*)AfxGetMainWnd())->m_DlgGame.Invalidate();

	return 0;
}

UINT32 CClientCmdHandler::OnCmdNearByRemove( UINT16 wCommandID, UINT64 u64ConnID, CBufferHelper *pBufferHelper )
{
	UINT32 dwCount = 0;
	pBufferHelper->Read(dwCount);

	printf("BEGIN---删除角色消息，预计删除人数:%d, 现在人数:%d", dwCount, m_PlayerObjMgr.size());

	for(UINT32 i = 0; i < dwCount; i++)
	{
		UINT64 u64CharID = 0;

		pBufferHelper->Read(u64CharID);

		CPlayerObject *pObj = m_PlayerObjMgr.GetPlayer(u64CharID);
		if(pObj == NULL)
		{
			ASSERT_FAIELD;
			return 0;
		}

		m_PlayerObjMgr.RemovePlayer(u64CharID);

		delete pObj;

		printf("删除角色:%d成功", (UINT32)u64CharID);

	}

	printf("END---删除角色消息");


	((CTestClientDlg*)AfxGetMainWnd())->m_DlgGame.Invalidate();

	return 0;
}

UINT32 CClientCmdHandler::OnCmdEnterGameAck( UINT16 wCommandID, UINT64 u64ConnID, CBufferHelper *pBufferHelper )
{
	StCharEnterGameAck CharEnterGameAck;

	pBufferHelper->Read(CharEnterGameAck);

	m_HostPlayer.ReadFromBuffer(pBufferHelper);

	printf("登录成功!");

	((CTestClientDlg*)AfxGetMainWnd())->m_DlgGame.Invalidate();

	return 0;
}

BOOL CClientCmdHandler::OnUpdate( UINT32 dwTick )
{

	return TRUE;
}

BOOL CClientCmdHandler::SendLoginReq( LPCTSTR szAccountName, LPCTSTR szPassword )
{
	return TRUE;
}

BOOL CClientCmdHandler::SendNewAccountReq( LPCTSTR szAccountName, LPCTSTR szPassword )
{
	StCharNewAccountReq CharNewAccountReq;
	strncpy(CharNewAccountReq.szAccountName, szAccountName, 32);
	strncpy(CharNewAccountReq.szPassword, szPassword, 32);

	CBufferHelper WriteHelper(TRUE, CNetworkMgr::GetInstancePtr()->m_pWriteBuffer);

	WriteHelper.BeginWrite(CMD_CHAR_NEW_ACCOUNT_REQ, 0, 0, 0);

	WriteHelper.Write(CharNewAccountReq);

	WriteHelper.EndWrite();

	CNetworkMgr::GetInstancePtr()->SendData(CNetworkMgr::GetInstancePtr()->m_pWriteBuffer->GetData(), CNetworkMgr::GetInstancePtr()->m_pWriteBuffer->GetDataLenth());


	return TRUE;
}

BOOL CClientCmdHandler::SendPickCharReq( UINT64 u64CharID )
{
	return TRUE;
}

UINT32 CClientCmdHandler::OnCmdNewAccountAck( UINT16 wCommandID, UINT64 u64ConnID, CBufferHelper *pBufferHelper )
{
	return 0;
}

UINT32 CClientCmdHandler::OnCmdNewCharAck( UINT16 wCommandID, UINT64 u64ConnID, CBufferHelper *pBufferHelper )
{
	return 0;
}

UINT32 CClientCmdHandler::OnCmdPickCharAck( UINT16 wCommandID, UINT64 u64ConnID, CBufferHelper *pBufferHelper )
{


	return 0;
}
