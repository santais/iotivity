//******************************************************************
//
// Copyright 2015 Intel Mobile Communications GmbH All Rights Reserved.
//
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#ifndef IOTVT_SRM_CREDR_H
#define IOTVT_SRM_CREDR_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initialize credential resource by loading data from persistent storage.
 *
 * @retval
 *     OC_STACK_OK    - no errors
 *     OC_STACK_ERROR - stack process error
 */
OCStackResult InitCredResource();

/**
 * Perform cleanup for credential resources.
 *
 * @retval
 *     OC_STACK_OK              - no errors
 *     OC_STACK_ERROR           - stack process error
 *     OC_STACK_NO_RESOURCE     - resource not found
 *     OC_STACK_INVALID_PARAM   - invalid param
 */
OCStackResult DeInitCredResource();

/**
 * This method is used by tinydtls/SRM to retrieve credential for given Subject.
 *
 * @param subject - subject for which credential is required.
 *
 * @retval
 *     reference to OicSecCred_t - if credential is found
 *     NULL                      - if credential not found
 */
const OicSecCred_t* GetCredResourceData(const OicUuid_t* subjectId);

/**
 * This function converts credential data into JSON format.
 * Caller needs to invoke 'free' when done using
 * returned string.
 * @param cred  pointer to instance of OicSecCred_t structure.
 *
 * @retval
 *      pointer to JSON credential representation - if credential for subjectId found
 *      NULL                                      - if credential for subjectId not found
 */
char* BinToCredJSON(const OicSecCred_t* cred);

/**
 * This function generates the bin credential data.
 *
 * @param subject pointer to subject of this credential.
 * @param credType credential type.
 * @param publicData public data such as public key.
 * @param privateData private data such as private key.
 * @param ownersLen length of owners array
 * @param owners array of owners.
 *
 * @retval
 *      pointer to instance of OicSecCred_t  - success
 *      NULL                                 - error
 */
OicSecCred_t * GenerateCredential(const OicUuid_t* subject, OicSecCredType_t credType,
                     const char * publicData, const char * privateData, size_t ownersLen,
                     const OicUuid_t * owners);

/**
 * This function adds the new cred to the credential list.
 *
 * @param cred pointer to new credential.
 *
 * @retval
 *      OC_STACK_OK     - cred not NULL and persistent storage gets updated
 *      OC_STACK_ERROR  - cred is NULL or fails to update persistent storage
 */
OCStackResult AddCredential(OicSecCred_t * cred);
#ifdef __cplusplus
}
#endif

#endif //IOTVT_SRM_CREDR_H

