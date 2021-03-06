/*
 * //******************************************************************
 * //
 * // Copyright 2016 Samsung Electronics All Rights Reserved.
 * //
 * //-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * //
 * // Licensed under the Apache License, Version 2.0 (the "License");
 * // you may not use this file except in compliance with the License.
 * // You may obtain a copy of the License at
 * //
 * //      http://www.apache.org/licenses/LICENSE-2.0
 * //
 * // Unless required by applicable law or agreed to in writing, software
 * // distributed under the License is distributed on an "AS IS" BASIS,
 * // WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * // See the License for the specific language governing permissions and
 * // limitations under the License.
 * //
 * //-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 */
package org.iotivity.cloud.accountserver.db;

import java.util.ArrayList;

import org.bson.Document;
import org.iotivity.cloud.accountserver.Const;
import org.iotivity.cloud.util.Logger;

import com.mongodb.MongoClient;
import com.mongodb.client.MongoCollection;
import com.mongodb.client.MongoCursor;
import com.mongodb.client.MongoDatabase;
import com.mongodb.client.model.Filters;

/**
 *
 * This class provides a set of APIs to use MongoDB APIs.
 *
 */
public class MongoDB {

    private MongoClient   mongoClient = null;
    private MongoDatabase db          = null;

    /**
     * API creating MongoClient and initializing MongoDatabase
     * 
     * @param dbname
     *            database name to create MongoDatabase
     * @throws Exception
     */
    public MongoDB(String dbname) throws Exception {
        mongoClient = new MongoClient();
        mongoClient.dropDatabase(dbname);
        db = mongoClient.getDatabase(dbname);
    }

    /**
     * API creating collection
     * 
     * @param tableName
     *            collection name
     */
    public void createTable(String tableName) {
        db.createCollection(tableName);
    }

    /**
     * API deleting collection
     * 
     * @param tableName
     *            collection name
     */
    public void deleteTable(String tableName) {
        db.getCollection(tableName).drop();
    }

    public MongoDatabase getMongoDatabase() {
        return db;
    }

    /**
     * API for storing information of authorized users
     * 
     * @param accountInfo
     *            information of authorized users
     * @param tablename
     *            table name of mongoDB
     */
    public void createResource(UserSession userSession) {

        Document doc = createDocument(userSession);
        MongoCollection<Document> collection = db
                .getCollection(Const.SESSION_TABLE);

        if (collection.findOneAndReplace(Filters.and(
                Filters.eq(Const.USER_ID, doc.get(Const.USER_ID)),
                Filters.eq(Const.SESSION_CODE, doc.get(Const.SESSION_CODE))),
                doc) == null) {

            collection.insertOne(doc);
        }

        return;
    }

    public void createResource(UserDevice userDevice) {

        Document doc = createDocument(userDevice);
        MongoCollection<Document> collection = db
                .getCollection(Const.DEVICE_TABLE);

        if (collection.findOneAndReplace(
                Filters.and(Filters.eq(Const.USER_ID, doc.get(Const.USER_ID)),
                        Filters.eq(Const.DEVICE_ID, doc.get(Const.DEVICE_ID))),
                doc) == null) {

            collection.insertOne(doc);
        }

        return;
    }

    private Document createDocument(UserSession userSession) {

        Document doc = new Document(Const.USER_ID, userSession.getUserId())
                .append(Const.SESSION_CODE, userSession.getSessionCode());

        return doc;
    }

    private Document createDocument(UserDevice userDevice) {

        Document doc = new Document(Const.USER_ID, userDevice.getUserId())
                .append(Const.DEVICE_ID, userDevice.getDeviceId());

        return doc;
    }

    private UserSession convertSessionDocToResource(Document doc) {

        UserSession userSession = new UserSession();

        userSession.setUserId(doc.getString(Const.USER_ID));
        userSession.setSessionCode(doc.getString(Const.SESSION_CODE));

        return userSession;
    }

    private UserDevice convertDeviceDocToResource(Document doc) {

        UserDevice userDevice = new UserDevice();

        userDevice.setUserId(doc.getString(Const.USER_ID));
        userDevice.setDeviceId(doc.getString(Const.DEVICE_ID));

        return userDevice;
    }

    public String getUserId(String sessionCode) {

        String userId = null;

        MongoCollection<Document> collection = db
                .getCollection(Const.SESSION_TABLE);

        MongoCursor<Document> cursor = collection
                .find(Filters.eq(Const.SESSION_CODE, sessionCode)).iterator();

        try {

            while (cursor.hasNext()) {

                Document doc = cursor.next();
                UserSession userSession = convertSessionDocToResource(doc);

                userId = userSession.getUserId();
                break;
            }

        } finally {

            cursor.close();
        }

        return userId;
    }

    /**
     * API for getting devices according to user from mongoDB
     * 
     * @param userId
     *            user identifier
     * @param tablename
     *            table name of mongoDB
     */
    public ArrayList<String> getDevices(String userId) {

        ArrayList<String> deviceList = new ArrayList<String>();

        MongoCollection<Document> collection = db
                .getCollection(Const.DEVICE_TABLE);

        MongoCursor<Document> cursor = collection
                .find(Filters.eq(Const.USER_ID, userId)).iterator();

        try {

            while (cursor.hasNext()) {

                Document doc = cursor.next();
                UserDevice userDeivce = convertDeviceDocToResource(doc);

                deviceList.add(userDeivce.getDeviceId());
            }

        } finally {

            cursor.close();
        }

        return deviceList;
    }

    private ArrayList<UserSession> readSessionResources() {

        ArrayList<UserSession> userSessionList = new ArrayList<UserSession>();

        MongoCollection<Document> collection = db
                .getCollection(Const.SESSION_TABLE);
        MongoCursor<Document> cursor = collection.find().iterator();

        while (cursor.hasNext()) {

            Document doc = cursor.next();
            userSessionList.add(convertSessionDocToResource(doc));
        }

        cursor.close();

        return userSessionList;
    }

    private ArrayList<UserDevice> readDeviceResources() {

        ArrayList<UserDevice> userDeviceList = new ArrayList<UserDevice>();

        MongoCollection<Document> collection = db
                .getCollection(Const.DEVICE_TABLE);
        MongoCursor<Document> cursor = collection.find().iterator();

        while (cursor.hasNext()) {

            Document doc = cursor.next();
            userDeviceList.add(convertDeviceDocToResource(doc));
        }

        cursor.close();

        return userDeviceList;
    }

    public void printResources() {

        ArrayList<UserDevice> dlist = readDeviceResources();
        int size = dlist.size();

        Logger.i("*Table: " + Const.DEVICE_TABLE);
        for (int i = 0; i < size; i++) {

            UserDevice item = dlist.get(i);

            Logger.i("[" + i + "]" + item.getUserId() + ", "
                    + item.getDeviceId());
        }

        ArrayList<UserSession> slist = readSessionResources();
        size = slist.size();

        Logger.i("*Table: " + Const.SESSION_TABLE);

        for (int i = 0; i < size; i++) {

            UserSession item = slist.get(i);

            Logger.i("[" + i + "]" + item.getUserId() + ", "
                    + item.getSessionCode());

        }
    }

}
