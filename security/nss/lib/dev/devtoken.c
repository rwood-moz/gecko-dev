/* 
 * The contents of this file are subject to the Mozilla Public
 * License Version 1.1 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of
 * the License at http://www.mozilla.org/MPL/
 * 
 * Software distributed under the License is distributed on an "AS
 * IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
 * implied. See the License for the specific language governing
 * rights and limitations under the License.
 * 
 * The Original Code is the Netscape security libraries.
 * 
 * The Initial Developer of the Original Code is Netscape
 * Communications Corporation.  Portions created by Netscape are 
 * Copyright (C) 1994-2000 Netscape Communications Corporation.  All
 * Rights Reserved.
 * 
 * Contributor(s):
 * 
 * Alternatively, the contents of this file may be used under the
 * terms of the GNU General Public License Version 2 or later (the
 * "GPL"), in which case the provisions of the GPL are applicable 
 * instead of those above.  If you wish to allow use of your 
 * version of this file only under the terms of the GPL and not to
 * allow others to use your version of this file under the MPL,
 * indicate your decision by deleting the provisions above and
 * replace them with the notice and other provisions required by
 * the GPL.  If you do not delete the provisions above, a recipient
 * may use your version of this file under either the MPL or the
 * GPL.
 */

#ifdef DEBUG
static const char CVS_ID[] = "@(#) $RCSfile: devtoken.c,v $ $Revision: 1.12 $ $Date: 2002/04/15 15:22:01 $ $Name:  $";
#endif /* DEBUG */

#ifndef NSSCKEPV_H
#include "nssckepv.h"
#endif /* NSSCKEPV_H */

#ifndef DEVM_H
#include "devm.h"
#endif /* DEVM_H */

#ifndef CKHELPER_H
#include "ckhelper.h"
#endif /* CKHELPER_H */

#ifdef NSS_3_4_CODE
#include "pk11func.h"
#include "dev3hack.h"
#endif

/* The number of object handles to grab during each call to C_FindObjects */
#define OBJECT_STACK_SIZE 16

#ifdef PURE_STAN_BUILD
struct NSSTokenStr
{
  struct nssDeviceBaseStr base;
  NSSSlot *slot;  /* Peer */
  CK_FLAGS ckFlags; /* from CK_TOKEN_INFO.flags */
  nssSession *defaultSession;
  nssTokenObjectCache *cache;
};

NSS_IMPLEMENT NSSToken *
nssToken_Create
(
  CK_SLOT_ID slotID,
  NSSSlot *peer
)
{
    NSSArena *arena;
    NSSToken *rvToken;
    nssSession *session = NULL;
    NSSUTF8 *tokenName = NULL;
    PRUint32 length;
    PRBool readWrite;
    CK_TOKEN_INFO tokenInfo;
    CK_RV ckrv;
    void *epv = nssSlot_GetCryptokiEPV(peer);
    arena = NSSArena_Create();
    if(!arena) {
	return (NSSToken *)NULL;
    }
    rvToken = nss_ZNEW(arena, NSSToken);
    if (!rvToken) {
	goto loser;
    }
    /* Get token information */
    ckrv = CKAPI(epv)->C_GetTokenInfo(slotID, &tokenInfo);
    if (ckrv != CKR_OK) {
	/* set an error here, eh? */
	goto loser;
    }
    /* Grab the slot description from the PKCS#11 fixed-length buffer */
    length = nssPKCS11String_Length(tokenInfo.label, sizeof(tokenInfo.label));
    if (length > 0) {
	tokenName = nssUTF8_Create(arena, nssStringType_UTF8String, 
	                           (void *)tokenInfo.label, length);
	if (!tokenName) {
	    goto loser;
	}
    }
    /* Open a default session handle for the token. */
    if (tokenInfo.ulMaxSessionCount == 1) {
	/* if the token can only handle one session, it must be RW. */
	readWrite = PR_TRUE;
    } else {
	readWrite = PR_FALSE;
    }
    session = nssSlot_CreateSession(peer, arena, readWrite);
    if (session == NULL) {
	goto loser;
    }
    /* TODO: seed the RNG here */
    rvToken->base.arena = arena;
    rvToken->base.refCount = 1;
    rvToken->base.name = tokenName;
    rvToken->base.lock = PZ_NewLock(nssNSSILockOther); /* XXX */
    if (!rvToken->base.lock) {
	goto loser;
    }
    rvToken->slot = peer; /* slot owns ref to token */
    rvToken->ckFlags = tokenInfo.flags;
    rvToken->defaultSession = session;
    if (nssSlot_IsHardware(peer)) {
	rvToken->cache = nssTokenObjectCache_Create(rvToken, 
	                                            PR_TRUE, PR_TRUE, PR_TRUE);
	if (!rvToken->cache) {
	    nssSlot_Destroy(peer);
	    goto loser;
	}
    }
    return rvToken;
loser:
    if (session) {
	nssSession_Destroy(session);
    }
    nssArena_Destroy(arena);
    return (NSSToken *)NULL;
}
#endif /* PURE_STAN_BUILD */

NSS_IMPLEMENT PRStatus
nssToken_Destroy
(
  NSSToken *tok
)
{
#ifdef PURE_STAN_BUILD
    PR_AtomicDecrement(&tok->base.refCount);
    if (tok->base.refCount == 0) {
	nssTokenObjectCache_Destroy(tok->cache);
	return nssArena_Destroy(tok->base.arena);
    }
#endif
    return PR_SUCCESS;
}

NSS_IMPLEMENT void
NSSToken_Destroy
(
  NSSToken *tok
)
{
    (void)nssToken_Destroy(tok);
}

NSS_IMPLEMENT NSSToken *
nssToken_AddRef
(
  NSSToken *tok
)
{
    PR_AtomicIncrement(&tok->base.refCount);
    return tok;
}

NSS_IMPLEMENT NSSSlot *
nssToken_GetSlot
(
  NSSToken *tok
)
{
    return nssSlot_AddRef(tok->slot);
}

#ifdef PURE_STAN_BUILD
NSS_IMPLEMENT NSSModule *
nssToken_GetModule
(
  NSSToken *token
)
{
    return nssSlot_GetModule(token->slot);
}
#endif

NSS_IMPLEMENT void *
nssToken_GetCryptokiEPV
(
  NSSToken *token
)
{
    return nssSlot_GetCryptokiEPV(token->slot);
}

NSS_IMPLEMENT nssSession *
nssToken_GetDefaultSession
(
  NSSToken *token
)
{
    return token->defaultSession;
}

NSS_IMPLEMENT NSSUTF8 *
nssToken_GetName
(
  NSSToken *tok
)
{
    if (tok->base.name[0] == 0) {
	(void) nssSlot_IsTokenPresent(tok->slot);
    } 
    return tok->base.name;
}

NSS_IMPLEMENT NSSUTF8 *
NSSToken_GetName
(
  NSSToken *token
)
{
    return nssToken_GetName(token);
}

NSS_IMPLEMENT PRBool
nssToken_IsLoginRequired
(
  NSSToken *token
)
{
    return (token->ckFlags & CKF_LOGIN_REQUIRED);
}

NSS_IMPLEMENT PRBool
nssToken_NeedsPINInitialization
(
  NSSToken *token
)
{
    return (!(token->ckFlags & CKF_USER_PIN_INITIALIZED));
}

NSS_IMPLEMENT PRStatus
nssToken_DeleteStoredObject
(
  nssCryptokiObject *instance
)
{
    CK_RV ckrv;
    PRStatus status;
    PRBool createdSession = PR_FALSE;
    NSSToken *token = instance->token;
    nssSession *session = NULL;
    void *epv = nssToken_GetCryptokiEPV(instance->token);
#ifdef PURE_STAN_BUILD
    if (token->cache) {
	status = nssTokenObjectCache_RemoveObject(token->cache, instance);
    }
#endif
    if (instance->isTokenObject) {
       if (nssSession_IsReadWrite(token->defaultSession)) {
	   session = token->defaultSession;
       } else {
	   session = nssSlot_CreateSession(token->slot, NULL, PR_TRUE);
	   createdSession = PR_TRUE;
       }
    }
    if (session == NULL) {
	return PR_FAILURE;
    }
    nssSession_EnterMonitor(session);
    ckrv = CKAPI(epv)->C_DestroyObject(session->handle, instance->handle);
    nssSession_ExitMonitor(session);
    if (createdSession) {
	nssSession_Destroy(session);
    }
    if (ckrv != CKR_OK) {
	return PR_FAILURE;
    }
    return status;
}

static nssCryptokiObject *
import_object
(
  NSSToken *tok,
  nssSession *sessionOpt,
  CK_ATTRIBUTE_PTR objectTemplate,
  CK_ULONG otsize
)
{
    nssSession *session = NULL;
    PRBool createdSession = PR_FALSE;
    nssCryptokiObject *object = NULL;
    CK_OBJECT_HANDLE handle;
    CK_RV ckrv;
    void *epv = nssToken_GetCryptokiEPV(tok);
    if (nssCKObject_IsTokenObjectTemplate(objectTemplate, otsize)) {
	if (sessionOpt) {
	    if (!nssSession_IsReadWrite(sessionOpt)) {
		return CK_INVALID_HANDLE;
	    } else {
		session = sessionOpt;
	    }
	} else if (nssSession_IsReadWrite(tok->defaultSession)) {
	    session = tok->defaultSession;
	} else {
	    session = nssSlot_CreateSession(tok->slot, NULL, PR_TRUE);
	    createdSession = PR_TRUE;
	}
    } else {
	session = (sessionOpt) ? sessionOpt : tok->defaultSession;
    }
    if (session == NULL) {
	return CK_INVALID_HANDLE;
    }
    nssSession_EnterMonitor(session);
    ckrv = CKAPI(epv)->C_CreateObject(session->handle, 
                                      objectTemplate, otsize,
                                      &handle);
    nssSession_ExitMonitor(session);
    if (ckrv == CKR_OK) {
	object = nssCryptokiObject_Create(tok, session, handle);
    }
    if (createdSession) {
	nssSession_Destroy(session);
    }
    return object;
}

static nssCryptokiObject **
create_objects_from_handles
(
  NSSToken *tok,
  nssSession *session,
  CK_OBJECT_HANDLE *handles,
  PRUint32 numH
)
{
    nssCryptokiObject **objects;
    objects = nss_ZNEWARRAY(NULL, nssCryptokiObject *, numH + 1);
    if (objects) {
	PRInt32 i;
	for (i=0; i<numH; i++) {
	    objects[i] = nssCryptokiObject_Create(tok, session, handles[i]);
	    if (!objects[i]) {
		for (--i; i>0; --i) {
		    nssCryptokiObject_Destroy(objects[i]);
		}
		return (nssCryptokiObject **)NULL;
	    }
	}
    }
    return objects;
}

static nssCryptokiObject **
find_objects
(
  NSSToken *tok,
  nssSession *sessionOpt,
  CK_ATTRIBUTE_PTR obj_template,
  CK_ULONG otsize,
  PRUint32 maximumOpt,
  PRStatus *statusOpt
)
{
    CK_RV ckrv;
    CK_ULONG count;
    CK_OBJECT_HANDLE *objectHandles;
    PRUint32 arraySize, numHandles;
    void *epv = nssToken_GetCryptokiEPV(tok);
    nssCryptokiObject **objects;
    NSSArena *arena;
    nssSession *session = (sessionOpt) ? sessionOpt : tok->defaultSession;
    /* the arena is only for the array of object handles */
    arena = nssArena_Create();
    if (!arena) {
	if (statusOpt) *statusOpt = PR_FAILURE;
	return (nssCryptokiObject **)NULL;
    }
    if (maximumOpt > 0) {
	arraySize = maximumOpt;
    } else {
	arraySize = OBJECT_STACK_SIZE;
    }
    numHandles = 0;
    objectHandles = nss_ZNEWARRAY(arena, CK_OBJECT_HANDLE, arraySize);
    if (!objectHandles) {
	goto loser;
    }
    nssSession_EnterMonitor(session); /* ==== session lock === */
    /* Initialize the find with the template */
    ckrv = CKAPI(epv)->C_FindObjectsInit(session->handle, 
                                         obj_template, otsize);
    if (ckrv != CKR_OK) {
	nssSession_ExitMonitor(session);
	goto loser;
    }
    while (PR_TRUE) {
	/* Issue the find for up to arraySize - numHandles objects */
	ckrv = CKAPI(epv)->C_FindObjects(session->handle, 
	                                 objectHandles + numHandles, 
	                                 arraySize - numHandles, 
	                                 &count);
	if (ckrv != CKR_OK) {
	    nssSession_ExitMonitor(session);
	    goto loser;
	}
	/* bump the number of found objects */
	numHandles += count;
	if (maximumOpt > 0 || numHandles < arraySize) {
	    /* When a maximum is provided, the search is done all at once,
	     * so the search is finished.  If the number returned was less 
	     * than the number sought, the search is finished.
	     */
	    break;
	}
	/* the array is filled, double it and continue */
	arraySize *= 2;
	objectHandles = nss_ZREALLOCARRAY(objectHandles, 
	                                  CK_OBJECT_HANDLE, 
	                                  arraySize);
	if (!objectHandles) {
	    nssSession_ExitMonitor(session);
	    goto loser;
	}
    }
    ckrv = CKAPI(epv)->C_FindObjectsFinal(session->handle);
    nssSession_ExitMonitor(session); /* ==== end session lock === */
    if (ckrv != CKR_OK) {
	goto loser;
    }
    if (numHandles > 0) {
	objects = create_objects_from_handles(tok, session,
	                                      objectHandles, numHandles);
    } else {
	objects = NULL;
    }
    nssArena_Destroy(arena);
    if (statusOpt) *statusOpt = PR_SUCCESS;
    return objects;
loser:
    nssArena_Destroy(arena);
    if (statusOpt) *statusOpt = PR_FAILURE;
    return (nssCryptokiObject **)NULL;
}

static nssCryptokiObject **
find_objects_by_template
(
  NSSToken *token,
  nssSession *sessionOpt,
  CK_ATTRIBUTE_PTR obj_template,
  CK_ULONG otsize,
  PRUint32 maximumOpt,
  PRStatus *statusOpt
)
{
    CK_OBJECT_CLASS objclass;
    nssCryptokiObject **objects = NULL;
    PRUint32 i;
#ifdef PURE_STAN_BUILD
    for (i=0; i<otsize; i++) {
	if (obj_template[i].type == CKA_CLASS) {
	    objclass = *(CK_OBJECT_CLASS *)obj_template[i].pValue;
	    break;
	}
    }
    PR_ASSERT(i < otsize);
    /* If these objects are being cached, try looking there first */
    if (token->cache && 
        nssTokenObjectCache_HaveObjectClass(token->cache, objclass)) 
    {
	objects = nssTokenObjectCache_FindObjectsByTemplate(token->cache,
	                                                    objclass,
	                                                    obj_template,
	                                                    otsize,
	                                                    maximumOpt);
	if (statusOpt) *statusOpt = PR_SUCCESS;
    }
#endif /* PURE_STAN_BUILD */
    /* Either they are not cached, or cache failed; look on token. */
    if (!objects) {
	objects = find_objects(token, sessionOpt, 
	                       obj_template, otsize, 
	                       maximumOpt, statusOpt);
    }
    return objects;
}

NSS_IMPLEMENT nssCryptokiObject *
nssToken_ImportCertificate
(
  NSSToken *tok,
  nssSession *sessionOpt,
  NSSCertificateType certType,
  NSSItem *id,
  NSSUTF8 *nickname,
  NSSDER *encoding,
  NSSDER *issuer,
  NSSDER *subject,
  NSSDER *serial,
  PRBool asTokenObject
)
{
    CK_CERTIFICATE_TYPE cert_type;
    CK_ATTRIBUTE_PTR attr;
    CK_ATTRIBUTE cert_tmpl[9];
    CK_ULONG ctsize;
    nssCryptokiObject *rvObject = NULL;
    if (certType == NSSCertificateType_PKIX) {
	cert_type = CKC_X_509;
    } else {
	return (nssCryptokiObject *)NULL;
    }
    NSS_CK_TEMPLATE_START(cert_tmpl, attr, ctsize);
    if (asTokenObject) {
	NSS_CK_SET_ATTRIBUTE_ITEM(attr, CKA_TOKEN, &g_ck_true);
    } else {
	NSS_CK_SET_ATTRIBUTE_ITEM(attr, CKA_TOKEN, &g_ck_false);
    }
    NSS_CK_SET_ATTRIBUTE_ITEM(attr, CKA_CLASS,            &g_ck_class_cert);
    NSS_CK_SET_ATTRIBUTE_VAR( attr, CKA_CERTIFICATE_TYPE,  cert_type);
    NSS_CK_SET_ATTRIBUTE_ITEM(attr, CKA_ID,                id);
    NSS_CK_SET_ATTRIBUTE_UTF8(attr, CKA_LABEL,             nickname);
    NSS_CK_SET_ATTRIBUTE_ITEM(attr, CKA_VALUE,             encoding);
    NSS_CK_SET_ATTRIBUTE_ITEM(attr, CKA_ISSUER,            issuer);
    NSS_CK_SET_ATTRIBUTE_ITEM(attr, CKA_SUBJECT,           subject);
    NSS_CK_SET_ATTRIBUTE_ITEM(attr, CKA_SERIAL_NUMBER,     serial);
    NSS_CK_TEMPLATE_FINISH(cert_tmpl, attr, ctsize);
    /* Import the certificate onto the token */
    rvObject = import_object(tok, sessionOpt, cert_tmpl, ctsize);
#ifdef PURE_STAN_BUILD
    if (rvObject && tok->cache) {
	nssTokenObjectCache_ImportObject(tok->cache, rvObject,
	                                 CKO_CERTIFICATE,
	                                 cert_tmpl, ctsize);
    }
#endif
    return rvObject;
}

/* traverse all certificates - this should only happen if the token
 * has been marked as "traversable"
 */
NSS_IMPLEMENT nssCryptokiObject **
nssToken_FindCertificates
(
  NSSToken *token,
  nssSession *sessionOpt,
  nssTokenSearchType searchType,
  PRUint32 maximumOpt,
  PRStatus *statusOpt
)
{
    CK_ATTRIBUTE_PTR attr;
    CK_ATTRIBUTE cert_template[2];
    CK_ULONG ctsize;
    nssCryptokiObject **objects;
    NSS_CK_TEMPLATE_START(cert_template, attr, ctsize);
    /* Set the search to token/session only if provided */
    if (searchType == nssTokenSearchType_SessionOnly) {
	NSS_CK_SET_ATTRIBUTE_ITEM(attr, CKA_TOKEN, &g_ck_false);
    } else if (searchType == nssTokenSearchType_TokenOnly ||
               searchType == nssTokenSearchType_TokenForced) {
	NSS_CK_SET_ATTRIBUTE_ITEM(attr, CKA_TOKEN, &g_ck_true);
    }
    NSS_CK_SET_ATTRIBUTE_ITEM(attr, CKA_CLASS, &g_ck_class_cert);
    NSS_CK_TEMPLATE_FINISH(cert_template, attr, ctsize);

    if (searchType == nssTokenSearchType_TokenForced) {
	objects = find_objects(token, sessionOpt,
	                       cert_template, ctsize,
	                       maximumOpt, statusOpt);
    } else {
	objects = find_objects_by_template(token, sessionOpt,
	                                   cert_template, ctsize,
	                                   maximumOpt, statusOpt);
    }
    return objects;
}

NSS_IMPLEMENT nssCryptokiObject **
nssToken_FindCertificatesBySubject
(
  NSSToken *token,
  nssSession *sessionOpt,
  NSSDER *subject,
  nssTokenSearchType searchType,
  PRUint32 maximumOpt,
  PRStatus *statusOpt
)
{
    CK_ATTRIBUTE_PTR attr;
    CK_ATTRIBUTE subj_template[3];
    CK_ULONG stsize;
    nssCryptokiObject **objects;
    NSS_CK_TEMPLATE_START(subj_template, attr, stsize);
    /* Set the search to token/session only if provided */
    if (searchType == nssTokenSearchType_SessionOnly) {
	NSS_CK_SET_ATTRIBUTE_ITEM(attr, CKA_TOKEN, &g_ck_false);
    } else if (searchType == nssTokenSearchType_TokenOnly) {
	NSS_CK_SET_ATTRIBUTE_ITEM(attr, CKA_TOKEN, &g_ck_true);
    }
    NSS_CK_SET_ATTRIBUTE_ITEM(attr, CKA_CLASS, &g_ck_class_cert);
    NSS_CK_SET_ATTRIBUTE_ITEM(attr, CKA_SUBJECT, subject);
    NSS_CK_TEMPLATE_FINISH(subj_template, attr, stsize);
    /* now locate the token certs matching this template */
    objects = find_objects_by_template(token, sessionOpt,
                                       subj_template, stsize,
                                       maximumOpt, statusOpt);
    return objects;
}

NSS_IMPLEMENT nssCryptokiObject **
nssToken_FindCertificatesByNickname
(
  NSSToken *token,
  nssSession *sessionOpt,
  NSSUTF8 *name,
  nssTokenSearchType searchType,
  PRUint32 maximumOpt,
  PRStatus *statusOpt
)
{
    CK_ATTRIBUTE_PTR attr;
    CK_ATTRIBUTE nick_template[3];
    CK_ULONG ntsize;
    nssCryptokiObject **objects;
    NSS_CK_TEMPLATE_START(nick_template, attr, ntsize);
    NSS_CK_SET_ATTRIBUTE_UTF8(attr, CKA_LABEL, name);
    /* Set the search to token/session only if provided */
    if (searchType == nssTokenSearchType_SessionOnly) {
	NSS_CK_SET_ATTRIBUTE_ITEM(attr, CKA_TOKEN, &g_ck_false);
    } else if (searchType == nssTokenSearchType_TokenOnly) {
	NSS_CK_SET_ATTRIBUTE_ITEM(attr, CKA_TOKEN, &g_ck_true);
    }
    NSS_CK_SET_ATTRIBUTE_ITEM(attr, CKA_CLASS, &g_ck_class_cert);
    NSS_CK_TEMPLATE_FINISH(nick_template, attr, ntsize);
    /* now locate the token certs matching this template */
    objects = find_objects_by_template(token, sessionOpt,
                                       nick_template, ntsize, 
                                       maximumOpt, statusOpt);
    if (!objects) {
	/* This is to workaround the fact that PKCS#11 doesn't specify
	 * whether the '\0' should be included.  XXX Is that still true?
	 * im - this is not needed by the current softoken.  However, I'm 
	 * leaving it in until I have surveyed more tokens to see if it needed.
	 * well, its needed by the builtin token...
	 */
	nick_template[0].ulValueLen++;
	objects = find_objects_by_template(token, sessionOpt,
	                                   nick_template, ntsize, 
	                                   maximumOpt, statusOpt);
    }
    return objects;
}

NSS_IMPLEMENT nssCryptokiObject **
nssToken_FindCertificatesByEmail
(
  NSSToken *token,
  nssSession *sessionOpt,
  NSSASCII7 *email,
  nssTokenSearchType searchType,
  PRUint32 maximumOpt,
  PRStatus *statusOpt
)
{
    CK_ATTRIBUTE_PTR attr;
    CK_ATTRIBUTE email_template[3];
    CK_ULONG etsize;
    nssCryptokiObject **objects;
    NSS_CK_TEMPLATE_START(email_template, attr, etsize);
    NSS_CK_SET_ATTRIBUTE_UTF8(attr, CKA_NETSCAPE_EMAIL, email);
    /* Set the search to token/session only if provided */
    if (searchType == nssTokenSearchType_SessionOnly) {
	NSS_CK_SET_ATTRIBUTE_ITEM(attr, CKA_TOKEN, &g_ck_false);
    } else if (searchType == nssTokenSearchType_TokenOnly) {
	NSS_CK_SET_ATTRIBUTE_ITEM(attr, CKA_TOKEN, &g_ck_true);
    }
    NSS_CK_SET_ATTRIBUTE_ITEM(attr, CKA_CLASS, &g_ck_class_cert);
    NSS_CK_TEMPLATE_FINISH(email_template, attr, etsize);
    /* now locate the token certs matching this template */
    objects = find_objects_by_template(token, sessionOpt,
                                       email_template, etsize,
                                       maximumOpt, statusOpt);
    if (!objects) {
	/* This is to workaround the fact that PKCS#11 doesn't specify
	 * whether the '\0' should be included.  XXX Is that still true?
	 * im - this is not needed by the current softoken.  However, I'm 
	 * leaving it in until I have surveyed more tokens to see if it needed.
	 * well, its needed by the builtin token...
	 */
	email_template[0].ulValueLen++;
	objects = find_objects_by_template(token, sessionOpt,
	                                   email_template, etsize,
	                                   maximumOpt, statusOpt);
    }
    return objects;
}

NSS_IMPLEMENT nssCryptokiObject **
nssToken_FindCertificatesByID
(
  NSSToken *token,
  nssSession *sessionOpt,
  NSSItem *id,
  nssTokenSearchType searchType,
  PRUint32 maximumOpt,
  PRStatus *statusOpt
)
{
    CK_ATTRIBUTE_PTR attr;
    CK_ATTRIBUTE id_template[3];
    CK_ULONG idtsize;
    nssCryptokiObject **objects;
    NSS_CK_TEMPLATE_START(id_template, attr, idtsize);
    NSS_CK_SET_ATTRIBUTE_ITEM(attr, CKA_ID, id);
    /* Set the search to token/session only if provided */
    if (searchType == nssTokenSearchType_SessionOnly) {
	NSS_CK_SET_ATTRIBUTE_ITEM(attr, CKA_TOKEN, &g_ck_false);
    } else if (searchType == nssTokenSearchType_TokenOnly) {
	NSS_CK_SET_ATTRIBUTE_ITEM(attr, CKA_TOKEN, &g_ck_true);
    }
    NSS_CK_SET_ATTRIBUTE_ITEM(attr, CKA_CLASS, &g_ck_class_cert);
    NSS_CK_TEMPLATE_FINISH(id_template, attr, idtsize);
    /* now locate the token certs matching this template */
    objects = find_objects_by_template(token, sessionOpt,
                                       id_template, idtsize,
                                       maximumOpt, statusOpt);
    return objects;
}

NSS_IMPLEMENT nssCryptokiObject *
nssToken_FindCertificateByIssuerAndSerialNumber
(
  NSSToken *token,
  nssSession *sessionOpt,
  NSSDER *issuer,
  NSSDER *serial,
  nssTokenSearchType searchType,
  PRStatus *statusOpt
)
{
    CK_ATTRIBUTE_PTR attr;
    CK_ATTRIBUTE cert_template[4];
    CK_ULONG ctsize;
    nssCryptokiObject **objects;
    nssCryptokiObject *rvObject = NULL;
    NSS_CK_TEMPLATE_START(cert_template, attr, ctsize);
    /* Set the search to token/session only if provided */
    if (searchType == nssTokenSearchType_SessionOnly) {
	NSS_CK_SET_ATTRIBUTE_ITEM(attr, CKA_TOKEN, &g_ck_false);
    } else if (searchType == nssTokenSearchType_TokenOnly) {
	NSS_CK_SET_ATTRIBUTE_ITEM(attr, CKA_TOKEN, &g_ck_true);
    }
    /* Set the unique id */
    NSS_CK_SET_ATTRIBUTE_ITEM(attr, CKA_CLASS,         &g_ck_class_cert);
    NSS_CK_SET_ATTRIBUTE_ITEM(attr, CKA_ISSUER,         issuer);
    NSS_CK_SET_ATTRIBUTE_ITEM(attr, CKA_SERIAL_NUMBER,  serial);
    NSS_CK_TEMPLATE_FINISH(cert_template, attr, ctsize);
    /* get the object handle */
    objects = find_objects_by_template(token, sessionOpt,
                                       cert_template, ctsize,
                                       1, statusOpt);
    if (objects) {
	rvObject = objects[0];
	nss_ZFreeIf(objects);
    }
    return rvObject;
}

NSS_IMPLEMENT nssCryptokiObject *
nssToken_FindCertificateByEncodedCertificate
(
  NSSToken *token,
  nssSession *sessionOpt,
  NSSBER *encodedCertificate,
  nssTokenSearchType searchType,
  PRStatus *statusOpt
)
{
    CK_ATTRIBUTE_PTR attr;
    CK_ATTRIBUTE cert_template[3];
    CK_ULONG ctsize;
    nssCryptokiObject **objects;
    nssCryptokiObject *rvObject = NULL;
    NSS_CK_TEMPLATE_START(cert_template, attr, ctsize);
    /* Set the search to token/session only if provided */
    if (searchType == nssTokenSearchType_SessionOnly) {
	NSS_CK_SET_ATTRIBUTE_ITEM(attr, CKA_TOKEN, &g_ck_false);
    } else if (searchType == nssTokenSearchType_TokenOnly) {
	NSS_CK_SET_ATTRIBUTE_ITEM(attr, CKA_TOKEN, &g_ck_true);
    }
    NSS_CK_SET_ATTRIBUTE_ITEM(attr, CKA_CLASS, &g_ck_class_cert);
    NSS_CK_SET_ATTRIBUTE_ITEM(attr, CKA_VALUE, encodedCertificate);
    NSS_CK_TEMPLATE_FINISH(cert_template, attr, ctsize);
    /* get the object handle */
    objects = find_objects_by_template(token, sessionOpt,
                                       cert_template, ctsize,
                                       1, statusOpt);
    if (objects) {
	rvObject = objects[0];
	nss_ZFreeIf(objects);
    }
    return rvObject;
}

NSS_IMPLEMENT nssCryptokiObject **
nssToken_FindPrivateKeys
(
  NSSToken *token,
  nssSession *sessionOpt,
  nssTokenSearchType searchType,
  PRUint32 maximumOpt,
  PRStatus *statusOpt
)
{
    CK_ATTRIBUTE_PTR attr;
    CK_ATTRIBUTE key_template[2];
    CK_ULONG ktsize;
    nssCryptokiObject **objects;

    NSS_CK_TEMPLATE_START(key_template, attr, ktsize);
    NSS_CK_SET_ATTRIBUTE_ITEM(attr, CKA_CLASS, &g_ck_class_privkey);
    if (searchType == nssTokenSearchType_SessionOnly) {
	NSS_CK_SET_ATTRIBUTE_ITEM(attr, CKA_TOKEN, &g_ck_false);
    } else if (searchType == nssTokenSearchType_TokenOnly) {
	NSS_CK_SET_ATTRIBUTE_ITEM(attr, CKA_TOKEN, &g_ck_true);
    }
    NSS_CK_TEMPLATE_FINISH(key_template, attr, ktsize);

    objects = find_objects_by_template(token, sessionOpt,
                                       key_template, ktsize, 
                                       maximumOpt, statusOpt);
    return objects;
}

/* XXX ?there are no session cert objects, so only search token objects */
NSS_IMPLEMENT nssCryptokiObject *
nssToken_FindPrivateKeyByID
(
  NSSToken *token,
  nssSession *sessionOpt,
  NSSItem *keyID
)
{
    CK_ATTRIBUTE_PTR attr;
    CK_ATTRIBUTE key_template[3];
    CK_ULONG ktsize;
    nssCryptokiObject **objects;
    nssCryptokiObject *rvKey = NULL;

    NSS_CK_TEMPLATE_START(key_template, attr, ktsize);
    NSS_CK_SET_ATTRIBUTE_ITEM(attr, CKA_CLASS, &g_ck_class_privkey);
    NSS_CK_SET_ATTRIBUTE_ITEM(attr, CKA_TOKEN, &g_ck_true);
    NSS_CK_SET_ATTRIBUTE_ITEM(attr, CKA_ID, keyID);
    NSS_CK_TEMPLATE_FINISH(key_template, attr, ktsize);

    objects = find_objects_by_template(token, sessionOpt,
                                       key_template, ktsize, 
                                       1, NULL);
    if (objects) {
	rvKey = objects[0];
	nss_ZFreeIf(objects);
    }
    return rvKey;
}

/* XXX ?there are no session cert objects, so only search token objects */
NSS_IMPLEMENT nssCryptokiObject *
nssToken_FindPublicKeyByID
(
  NSSToken *token,
  nssSession *sessionOpt,
  NSSItem *keyID
)
{
    CK_ATTRIBUTE_PTR attr;
    CK_ATTRIBUTE key_template[3];
    CK_ULONG ktsize;
    nssCryptokiObject **objects;
    nssCryptokiObject *rvKey = NULL;

    NSS_CK_TEMPLATE_START(key_template, attr, ktsize);
    NSS_CK_SET_ATTRIBUTE_ITEM(attr, CKA_CLASS, &g_ck_class_pubkey);
    NSS_CK_SET_ATTRIBUTE_ITEM(attr, CKA_TOKEN, &g_ck_true);
    NSS_CK_SET_ATTRIBUTE_ITEM(attr, CKA_ID, keyID);
    NSS_CK_TEMPLATE_FINISH(key_template, attr, ktsize);

    objects = find_objects_by_template(token, sessionOpt,
                                       key_template, ktsize, 
                                       1, NULL);
    if (objects) {
	rvKey = objects[0];
	nss_ZFreeIf(objects);
    }
    return rvKey;
}

static void
sha1_hash(NSSItem *input, NSSItem *output)
{
    NSSAlgorithmAndParameters *ap;
#ifdef NSS_3_4_CODE
    PK11SlotInfo *internal = PK11_GetInternalSlot();
    NSSToken *token = PK11Slot_GetNSSToken(internal);
#else
    NSSToken *token = nss_GetDefaultCryptoToken();
#endif
    ap = NSSAlgorithmAndParameters_CreateSHA1Digest(NULL);
    (void)nssToken_Digest(token, NULL, ap, input, output, NULL);
#ifdef NSS_3_4_CODE
    PK11_FreeSlot(token->pk11slot);
#endif
    nss_ZFreeIf(ap);
}

static void
md5_hash(NSSItem *input, NSSItem *output)
{
    NSSAlgorithmAndParameters *ap;
#ifdef NSS_3_4_CODE
    PK11SlotInfo *internal = PK11_GetInternalSlot();
    NSSToken *token = PK11Slot_GetNSSToken(internal);
#else
    NSSToken *token = nss_GetDefaultCryptoToken();
#endif
    ap = NSSAlgorithmAndParameters_CreateMD5Digest(NULL);
    (void)nssToken_Digest(token, NULL, ap, input, output, NULL);
#ifdef NSS_3_4_CODE
    PK11_FreeSlot(token->pk11slot);
#endif
    nss_ZFreeIf(ap);
}

static CK_TRUST
get_ck_trust
(
  nssTrustLevel nssTrust
)
{
    CK_TRUST t;
    switch (nssTrust) {
    case nssTrustLevel_Unknown: t = CKT_NETSCAPE_TRUST_UNKNOWN; break;
    case nssTrustLevel_NotTrusted: t = CKT_NETSCAPE_UNTRUSTED; break;
    case nssTrustLevel_TrustedDelegator: t = CKT_NETSCAPE_TRUSTED_DELEGATOR; 
	break;
    case nssTrustLevel_ValidDelegator: t = CKT_NETSCAPE_VALID_DELEGATOR; break;
    case nssTrustLevel_Trusted: t = CKT_NETSCAPE_TRUSTED; break;
    case nssTrustLevel_Valid: t = CKT_NETSCAPE_VALID; break;
    }
    return t;
}
 
NSS_IMPLEMENT nssCryptokiObject *
nssToken_ImportTrust
(
  NSSToken *tok,
  nssSession *sessionOpt,
  NSSDER *certEncoding,
  NSSDER *certIssuer,
  NSSDER *certSerial,
  nssTrustLevel serverAuth,
  nssTrustLevel clientAuth,
  nssTrustLevel codeSigning,
  nssTrustLevel emailProtection,
  PRBool asTokenObject
)
{
    nssCryptokiObject *object;
    CK_OBJECT_CLASS tobjc = CKO_NETSCAPE_TRUST;
    CK_TRUST ckSA, ckCA, ckCS, ckEP;
    CK_ATTRIBUTE_PTR attr;
    CK_ATTRIBUTE trust_tmpl[10];
    CK_ULONG tsize;
    PRUint8 sha1[20]; /* this is cheating... */
    PRUint8 md5[16];
    NSSItem sha1_result, md5_result;
    sha1_result.data = sha1; sha1_result.size = sizeof sha1;
    md5_result.data = md5; md5_result.size = sizeof md5;
    sha1_hash(certEncoding, &sha1_result);
    md5_hash(certEncoding, &md5_result);
    ckSA = get_ck_trust(serverAuth);
    ckCA = get_ck_trust(clientAuth);
    ckCS = get_ck_trust(codeSigning);
    ckEP = get_ck_trust(emailProtection);
    NSS_CK_TEMPLATE_START(trust_tmpl, attr, tsize);
    if (asTokenObject) {
	NSS_CK_SET_ATTRIBUTE_ITEM(attr, CKA_TOKEN, &g_ck_true);
    } else {
	NSS_CK_SET_ATTRIBUTE_ITEM(attr, CKA_TOKEN, &g_ck_false);
    }
    NSS_CK_SET_ATTRIBUTE_VAR( attr, CKA_CLASS,           tobjc);
    NSS_CK_SET_ATTRIBUTE_ITEM(attr, CKA_ISSUER,          certIssuer);
    NSS_CK_SET_ATTRIBUTE_ITEM(attr, CKA_SERIAL_NUMBER,   certSerial);
    NSS_CK_SET_ATTRIBUTE_ITEM(attr, CKA_CERT_SHA1_HASH, &sha1_result);
    NSS_CK_SET_ATTRIBUTE_ITEM(attr, CKA_CERT_MD5_HASH,  &md5_result);
    /* now set the trust values */
    NSS_CK_SET_ATTRIBUTE_VAR(attr, CKA_TRUST_SERVER_AUTH,      ckSA);
    NSS_CK_SET_ATTRIBUTE_VAR(attr, CKA_TRUST_CLIENT_AUTH,      ckCA);
    NSS_CK_SET_ATTRIBUTE_VAR(attr, CKA_TRUST_CODE_SIGNING,     ckCS);
    NSS_CK_SET_ATTRIBUTE_VAR(attr, CKA_TRUST_EMAIL_PROTECTION, ckEP);
    NSS_CK_TEMPLATE_FINISH(trust_tmpl, attr, tsize);
    /* import the trust object onto the token */
    object = import_object(tok, sessionOpt, trust_tmpl, tsize);
#ifdef PURE_STAN_BUILD
    if (object && tok->cache) {
	nssTokenObjectCache_ImportObject(tok->cache, object,
	                                 CKO_CERTIFICATE,
	                                 trust_tmpl, tsize);
    }
#endif
    return object;
}

NSS_IMPLEMENT nssCryptokiObject **
nssToken_FindTrustObjects
(
  NSSToken *token,
  nssSession *sessionOpt,
  nssTokenSearchType searchType,
  PRUint32 maximumOpt,
  PRStatus *statusOpt
)
{
    CK_OBJECT_CLASS tobjc = CKO_NETSCAPE_TRUST;
    CK_ATTRIBUTE_PTR attr;
    CK_ATTRIBUTE tobj_template[2];
    CK_ULONG tobj_size;
    nssCryptokiObject **objects;
    nssSession *session = sessionOpt ? sessionOpt : token->defaultSession;

    NSS_CK_TEMPLATE_START(tobj_template, attr, tobj_size);
    if (searchType == nssTokenSearchType_SessionOnly) {
	NSS_CK_SET_ATTRIBUTE_ITEM(attr, CKA_TOKEN, &g_ck_false);
    } else if (searchType == nssTokenSearchType_TokenOnly ||
               searchType == nssTokenSearchType_TokenForced) {
	NSS_CK_SET_ATTRIBUTE_ITEM(attr, CKA_TOKEN, &g_ck_true);
    }
    NSS_CK_SET_ATTRIBUTE_VAR( attr, CKA_CLASS, tobjc);
    NSS_CK_TEMPLATE_FINISH(tobj_template, attr, tobj_size);

    if (searchType == nssTokenSearchType_TokenForced) {
	objects = find_objects(token, session,
	                       tobj_template, tobj_size,
	                       maximumOpt, statusOpt);
    } else {
	objects = find_objects_by_template(token, session,
	                                   tobj_template, tobj_size,
	                                   maximumOpt, statusOpt);
    }
    return objects;
}

NSS_IMPLEMENT nssCryptokiObject *
nssToken_FindTrustForCertificate
(
  NSSToken *token,
  nssSession *sessionOpt,
  NSSDER *certEncoding,
  NSSDER *certIssuer,
  NSSDER *certSerial,
  nssTokenSearchType searchType
)
{
    CK_OBJECT_CLASS tobjc = CKO_NETSCAPE_TRUST;
    CK_ATTRIBUTE_PTR attr;
    CK_ATTRIBUTE tobj_template[5];
    CK_ULONG tobj_size;
    PRUint8 sha1[20]; /* this is cheating... */
    NSSItem sha1_result;
    nssSession *session = sessionOpt ? sessionOpt : token->defaultSession;
    nssCryptokiObject *object, **objects;

    sha1_result.data = sha1; sha1_result.size = sizeof sha1;
    sha1_hash(certEncoding, &sha1_result);
    NSS_CK_TEMPLATE_START(tobj_template, attr, tobj_size);
    if (searchType == nssTokenSearchType_SessionOnly) {
	NSS_CK_SET_ATTRIBUTE_ITEM(attr, CKA_TOKEN, &g_ck_false);
    } else if (searchType == nssTokenSearchType_TokenOnly) {
	NSS_CK_SET_ATTRIBUTE_ITEM(attr, CKA_TOKEN, &g_ck_true);
    }
    NSS_CK_SET_ATTRIBUTE_VAR( attr, CKA_CLASS,          tobjc);
    NSS_CK_SET_ATTRIBUTE_ITEM(attr, CKA_CERT_SHA1_HASH, &sha1_result);
#ifdef NSS_3_4_CODE
    if (!PK11_HasRootCerts(token->pk11slot)) {
#endif
    NSS_CK_SET_ATTRIBUTE_ITEM(attr, CKA_ISSUER,         certIssuer);
    NSS_CK_SET_ATTRIBUTE_ITEM(attr, CKA_SERIAL_NUMBER , certSerial);
#ifdef NSS_3_4_CODE
    }
    /*
     * we need to arrange for the built-in token to lose the bottom 2 
     * attributes so that old built-in tokens will continue to work.
     */
#endif
    NSS_CK_TEMPLATE_FINISH(tobj_template, attr, tobj_size);
    object = NULL;
    objects = find_objects_by_template(token, session,
                                       tobj_template, tobj_size,
                                       1, NULL);
    if (objects) {
	object = objects[0];
	nss_ZFreeIf(objects);
    }
    return object;
}
 
NSS_IMPLEMENT nssCryptokiObject *
nssToken_ImportCRL
(
  NSSToken *token,
  nssSession *sessionOpt,
  NSSDER *subject,
  NSSDER *encoding,
  PRBool isKRL,
  NSSUTF8 *url,
  PRBool asTokenObject
)
{
    nssCryptokiObject *object;
    CK_OBJECT_CLASS crlobjc = CKO_NETSCAPE_CRL;
    CK_ATTRIBUTE_PTR attr;
    CK_ATTRIBUTE crl_tmpl[6];
    CK_ULONG crlsize;

    NSS_CK_TEMPLATE_START(crl_tmpl, attr, crlsize);
    if (asTokenObject) {
	NSS_CK_SET_ATTRIBUTE_ITEM(attr, CKA_TOKEN, &g_ck_true);
    } else {
	NSS_CK_SET_ATTRIBUTE_ITEM(attr, CKA_TOKEN, &g_ck_false);
    }
    NSS_CK_SET_ATTRIBUTE_VAR( attr, CKA_CLASS,        crlobjc);
    NSS_CK_SET_ATTRIBUTE_ITEM(attr, CKA_SUBJECT,      subject);
    NSS_CK_SET_ATTRIBUTE_ITEM(attr, CKA_VALUE,        encoding);
    NSS_CK_SET_ATTRIBUTE_UTF8(attr, CKA_NETSCAPE_URL, url);
    if (isKRL) {
	NSS_CK_SET_ATTRIBUTE_ITEM(attr, CKA_NETSCAPE_KRL, &g_ck_true);
    } else {
	NSS_CK_SET_ATTRIBUTE_ITEM(attr, CKA_NETSCAPE_KRL, &g_ck_false);
    }
    NSS_CK_TEMPLATE_FINISH(crl_tmpl, attr, crlsize);

    /* import the crl object onto the token */
    object = import_object(token, sessionOpt, crl_tmpl, crlsize);
#ifdef PURE_STAN_BUILD
    if (object && token->cache) {
	nssTokenObjectCache_ImportObject(token->cache, object,
	                                 CKO_CERTIFICATE,
	                                 crl_tmpl, crlsize);
    }
#endif
    return object;
}

NSS_IMPLEMENT nssCryptokiObject **
nssToken_FindCRLs
(
  NSSToken *token,
  nssSession *sessionOpt,
  nssTokenSearchType searchType,
  PRUint32 maximumOpt,
  PRStatus *statusOpt
)
{
    CK_OBJECT_CLASS crlobjc = CKO_NETSCAPE_CRL;
    CK_ATTRIBUTE_PTR attr;
    CK_ATTRIBUTE crlobj_template[2];
    CK_ULONG crlobj_size;
    nssCryptokiObject **objects;
    nssSession *session = sessionOpt ? sessionOpt : token->defaultSession;

    NSS_CK_TEMPLATE_START(crlobj_template, attr, crlobj_size);
    if (searchType == nssTokenSearchType_SessionOnly) {
	NSS_CK_SET_ATTRIBUTE_ITEM(attr, CKA_TOKEN, &g_ck_false);
    } else if (searchType == nssTokenSearchType_TokenOnly ||
               searchType == nssTokenSearchType_TokenForced) {
	NSS_CK_SET_ATTRIBUTE_ITEM(attr, CKA_TOKEN, &g_ck_true);
    }
    NSS_CK_SET_ATTRIBUTE_VAR( attr, CKA_CLASS, crlobjc);
    NSS_CK_TEMPLATE_FINISH(crlobj_template, attr, crlobj_size);

    if (searchType == nssTokenSearchType_TokenForced) {
	objects = find_objects(token, session,
	                       crlobj_template, crlobj_size,
	                       maximumOpt, statusOpt);
    } else {
	objects = find_objects_by_template(token, session,
	                                   crlobj_template, crlobj_size,
	                                   maximumOpt, statusOpt);
    }
    return objects;
}

#ifdef PURE_STAN_BUILD
NSS_IMPLEMENT PRStatus
nssToken_GetCachedObjectAttributes
(
  NSSToken *token,
  NSSArena *arenaOpt,
  nssCryptokiObject *object,
  CK_OBJECT_CLASS objclass,
  CK_ATTRIBUTE_PTR atemplate,
  CK_ULONG atlen
)
{
    if (!token->cache) {
	return PR_FAILURE;
    }
    return nssTokenObjectCache_GetObjectAttributes(token->cache, arenaOpt,
                                                   object, objclass,
                                                   atemplate, atlen);
}
#endif

NSS_IMPLEMENT NSSItem *
nssToken_Digest
(
  NSSToken *tok,
  nssSession *sessionOpt,
  NSSAlgorithmAndParameters *ap,
  NSSItem *data,
  NSSItem *rvOpt,
  NSSArena *arenaOpt
)
{
    CK_RV ckrv;
    CK_ULONG digestLen;
    CK_BYTE_PTR digest;
    NSSItem *rvItem = NULL;
    void *epv = nssToken_GetCryptokiEPV(tok);
    nssSession *session;
    session = (sessionOpt) ? sessionOpt : tok->defaultSession;
    nssSession_EnterMonitor(session);
    ckrv = CKAPI(epv)->C_DigestInit(session->handle, &ap->mechanism);
    if (ckrv != CKR_OK) {
	nssSession_ExitMonitor(session);
	return NULL;
    }
#if 0
    /* XXX the standard says this should work, but it doesn't */
    ckrv = CKAPI(epv)->C_Digest(session->handle, NULL, 0, NULL, &digestLen);
    if (ckrv != CKR_OK) {
	nssSession_ExitMonitor(session);
	return NULL;
    }
#endif
    digestLen = 0; /* XXX for now */
    digest = NULL;
    if (rvOpt) {
	if (rvOpt->size > 0 && rvOpt->size < digestLen) {
	    nssSession_ExitMonitor(session);
	    /* the error should be bad args */
	    return NULL;
	}
	if (rvOpt->data) {
	    digest = rvOpt->data;
	}
	digestLen = rvOpt->size;
    }
    if (!digest) {
	digest = (CK_BYTE_PTR)nss_ZAlloc(arenaOpt, digestLen);
	if (!digest) {
	    nssSession_ExitMonitor(session);
	    return NULL;
	}
    }
    ckrv = CKAPI(epv)->C_Digest(session->handle, 
                                (CK_BYTE_PTR)data->data, 
                                (CK_ULONG)data->size,
                                (CK_BYTE_PTR)digest,
                                &digestLen);
    nssSession_ExitMonitor(session);
    if (ckrv != CKR_OK) {
	nss_ZFreeIf(digest);
	return NULL;
    }
    if (!rvOpt) {
	rvItem = nssItem_Create(arenaOpt, NULL, digestLen, (void *)digest);
    }
    return rvItem;
}

NSS_IMPLEMENT PRStatus
nssToken_BeginDigest
(
  NSSToken *tok,
  nssSession *sessionOpt,
  NSSAlgorithmAndParameters *ap
)
{
    CK_RV ckrv;
    nssSession *session;
    void *epv = nssToken_GetCryptokiEPV(tok);
    session = (sessionOpt) ? sessionOpt : tok->defaultSession;
    nssSession_EnterMonitor(session);
    ckrv = CKAPI(epv)->C_DigestInit(session->handle, &ap->mechanism);
    nssSession_ExitMonitor(session);
    return (ckrv == CKR_OK) ? PR_SUCCESS : PR_FAILURE;
}

NSS_IMPLEMENT PRStatus
nssToken_ContinueDigest
(
  NSSToken *tok,
  nssSession *sessionOpt,
  NSSItem *item
)
{
    CK_RV ckrv;
    nssSession *session;
    void *epv = nssToken_GetCryptokiEPV(tok);
    session = (sessionOpt) ? sessionOpt : tok->defaultSession;
    nssSession_EnterMonitor(session);
    ckrv = CKAPI(epv)->C_DigestUpdate(session->handle, 
                                      (CK_BYTE_PTR)item->data, 
                                      (CK_ULONG)item->size);
    nssSession_ExitMonitor(session);
    return (ckrv == CKR_OK) ? PR_SUCCESS : PR_FAILURE;
}

NSS_IMPLEMENT NSSItem *
nssToken_FinishDigest
(
  NSSToken *tok,
  nssSession *sessionOpt,
  NSSItem *rvOpt,
  NSSArena *arenaOpt
)
{
    CK_RV ckrv;
    CK_ULONG digestLen;
    CK_BYTE_PTR digest;
    NSSItem *rvItem = NULL;
    void *epv = nssToken_GetCryptokiEPV(tok);
    nssSession *session;
    session = (sessionOpt) ? sessionOpt : tok->defaultSession;
    nssSession_EnterMonitor(session);
    ckrv = CKAPI(epv)->C_DigestFinal(session->handle, NULL, &digestLen);
    if (ckrv != CKR_OK || digestLen == 0) {
	nssSession_ExitMonitor(session);
	return NULL;
    }
    digest = NULL;
    if (rvOpt) {
	if (rvOpt->size > 0 && rvOpt->size < digestLen) {
	    nssSession_ExitMonitor(session);
	    /* the error should be bad args */
	    return NULL;
	}
	if (rvOpt->data) {
	    digest = rvOpt->data;
	}
	digestLen = rvOpt->size;
    }
    if (!digest) {
	digest = (CK_BYTE_PTR)nss_ZAlloc(arenaOpt, digestLen);
	if (!digest) {
	    nssSession_ExitMonitor(session);
	    return NULL;
	}
    }
    ckrv = CKAPI(epv)->C_DigestFinal(session->handle, digest, &digestLen);
    nssSession_ExitMonitor(session);
    if (ckrv != CKR_OK) {
	nss_ZFreeIf(digest);
	return NULL;
    }
    if (!rvOpt) {
	rvItem = nssItem_Create(arenaOpt, NULL, digestLen, (void *)digest);
    }
    return rvItem;
}

#ifdef NSS_3_4_CODE

NSS_IMPLEMENT PRStatus
nssToken_SetTrustCache
(
  NSSToken *token
)
{
    CK_OBJECT_CLASS tobjc = CKO_NETSCAPE_TRUST;
    CK_ATTRIBUTE_PTR attr;
    CK_ATTRIBUTE tobj_template[2];
    CK_ULONG tobj_size;
    nssCryptokiObject **objects;
    nssSession *session = token->defaultSession;

    NSS_CK_TEMPLATE_START(tobj_template, attr, tobj_size);
    NSS_CK_SET_ATTRIBUTE_VAR( attr, CKA_CLASS, tobjc);
    NSS_CK_SET_ATTRIBUTE_ITEM(attr, CKA_TOKEN, &g_ck_true);
    NSS_CK_TEMPLATE_FINISH(tobj_template, attr, tobj_size);

    objects = find_objects_by_template(token, session,
                                       tobj_template, tobj_size, 1, NULL);
    token->hasNoTrust = PR_FALSE;
    if (objects) {
	nssCryptokiObjectArray_Destroy(objects);
    } else {
	token->hasNoTrust = PR_TRUE;
    } 
    return PR_SUCCESS;
}

NSS_IMPLEMENT PRStatus
nssToken_SetCrlCache
(
  NSSToken *token
)
{
    CK_OBJECT_CLASS tobjc = CKO_NETSCAPE_CRL;
    CK_ATTRIBUTE_PTR attr;
    CK_ATTRIBUTE tobj_template[2];
    CK_ULONG tobj_size;
    nssCryptokiObject **objects;
    nssSession *session = token->defaultSession;

    NSS_CK_TEMPLATE_START(tobj_template, attr, tobj_size);
    NSS_CK_SET_ATTRIBUTE_VAR( attr, CKA_CLASS, tobjc);
    NSS_CK_SET_ATTRIBUTE_ITEM(attr, CKA_TOKEN, &g_ck_true);
    NSS_CK_TEMPLATE_FINISH(tobj_template, attr, tobj_size);

    objects = find_objects_by_template(token, session,
                                       tobj_template, tobj_size, 1, NULL);
    token->hasNoCrls = PR_TRUE;
    if (objects) {
	nssCryptokiObjectArray_Destroy(objects);
    } else {
	token->hasNoCrls = PR_TRUE;
    }
    return PR_SUCCESS;
}

NSS_IMPLEMENT PRBool
nssToken_HasCrls
(
    NSSToken *tok
)
{
    return !tok->hasNoCrls;
}

NSS_IMPLEMENT PRStatus
nssToken_SetHasCrls
(
    NSSToken *tok
)
{
    tok->hasNoCrls = PR_FALSE;
    return PR_SUCCESS;
}

NSS_IMPLEMENT PRBool
nssToken_IsPresent
(
  NSSToken *token
)
{
    return nssSlot_IsTokenPresent(token->slot);
}
#endif

