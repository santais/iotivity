/******************************************************************
 *
 * Copyright 2015 Samsung Electronics All Rights Reserved.
 *
 *
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 ******************************************************************/
/**
 * @file    IFindGroupListener.java
 *
 * @brief   This file provides interface for getting group discovery status.
 *
 */

package org.iotivity.service.tm;

import org.iotivity.base.OcResource;

/**
 * @interface IFindGroupListener
 * @brief Provides interface for getting group discovery status.
 */
public interface IFindGroupListener {
    /**
     * This callback method will be called to notify whether group is found or
     * not.
     *
     * @param resource
     *            - URI of resource.
     */
    public void onGroupFindCallback(OcResource resource);
}