#include <iostream>
#include <gtest/gtest.h>
#include <HippoMocks/hippomocks.h>

#include "ResourceCacheManager.h"
#include "DataCache.h"
#include "RCSResourceAttributes.h"
#include "ResponseStatement.h"
#include "UnitTestHelper.h"

using namespace OIC::Service;

class DataCacheTest : public TestWithMock
{
    public:
        typedef std::function <
        void(const OIC::Service::HeaderOptions &, const OIC::Service::ResponseStatement &,
             int) > GetCallback;

        typedef std::function <
        void(const OIC::Service::HeaderOptions &, const OIC::Service::ResponseStatement &, int,
             int) > ObserveCallback;
    public:
        DataCache *cacheHandler;
        PrimitiveResource::Ptr pResource;
        CacheCB cb;
        CacheID id;

    protected:
        DataCacheTest()
        {

        }

        virtual ~DataCacheTest() noexcept(true)
        {

        }

        virtual void SetUp()
        {
            TestWithMock::SetUp();
            pResource = PrimitiveResource::Ptr(mocks.Mock< PrimitiveResource >(), [](PrimitiveResource *) {});
            cacheHandler = new DataCache();
            cb = ([](std::shared_ptr<PrimitiveResource >, const RCSResourceAttributes &)->OCStackResult {return OC_STACK_OK;});
        }

        virtual void TearDown()
        {
            delete cacheHandler;
            TestWithMock::TearDown();
        }
};

TEST_F(DataCacheTest, initializeDataCache_normalCase)
{

    mocks.ExpectCall(pResource.get(), PrimitiveResource::requestGet);
    mocks.ExpectCall(pResource.get(), PrimitiveResource::isObservable).Return(true);
    mocks.ExpectCall(pResource.get(), PrimitiveResource::requestObserve);
    mocks.OnCall(pResource.get(), PrimitiveResource::cancelObserve);

    cacheHandler->initializeDataCache(pResource);
}

TEST_F(DataCacheTest, initializeDataCache_normalCaseObservable)
{

    mocks.OnCall(pResource.get(), PrimitiveResource::requestGet).Do(
        [](GetCallback callback)
    {
        OIC::Service::HeaderOptions hos;

        OIC::Service::RCSResourceAttributes attr;
        OIC::Service::ResponseStatement rep(attr);
        callback(hos, rep, OC_STACK_OK);
        return;
    }
    );
    mocks.OnCall(pResource.get(), PrimitiveResource::isObservable).Return(true);
    mocks.OnCall(pResource.get(), PrimitiveResource::requestObserve).Do(
        [](ObserveCallback callback)
    {
        OIC::Service::HeaderOptions hos;
        OIC::Service::RCSResourceAttributes attr;
        OIC::Service::ResponseStatement rep(attr);
        int seq;
        callback(hos, rep, OC_STACK_OK, seq);
        return;
    }
    );
    mocks.OnCall(pResource.get(), PrimitiveResource::cancelObserve);

    cacheHandler->initializeDataCache(pResource);
}

TEST_F(DataCacheTest, initializeDataCache_normalCaseNonObservable)
{

    mocks.OnCall(pResource.get(), PrimitiveResource::requestGet).Do(
        [](GetCallback callback)
    {
        OIC::Service::HeaderOptions hos;

        OIC::Service::RCSResourceAttributes attr;
        OIC::Service::ResponseStatement rep(attr);
        callback(hos, rep, OC_STACK_OK);
        return;
    }
    );
    mocks.OnCall(pResource.get(), PrimitiveResource::isObservable).Return(false);
    mocks.OnCall(pResource.get(), PrimitiveResource::cancelObserve);

    cacheHandler->initializeDataCache(pResource);

    sleep(3);
}

TEST_F(DataCacheTest, initializeDataCache_normalCaseTimeOut)
{

    mocks.ExpectCall(pResource.get(), PrimitiveResource::requestGet);
    mocks.ExpectCall(pResource.get(), PrimitiveResource::isObservable).Return(true);
    mocks.ExpectCall(pResource.get(), PrimitiveResource::requestObserve);
    mocks.OnCall(pResource.get(), PrimitiveResource::cancelObserve);

    cacheHandler->initializeDataCache(pResource);

    sleep(3);
}

TEST_F(DataCacheTest, addSubscriber_normalCase)
{

    mocks.ExpectCall(pResource.get(), PrimitiveResource::requestGet);
    mocks.ExpectCall(pResource.get(), PrimitiveResource::isObservable).Return(true);
    mocks.ExpectCall(pResource.get(), PrimitiveResource::requestObserve);
    mocks.OnCall(pResource.get(), PrimitiveResource::cancelObserve);

    cacheHandler->initializeDataCache(pResource);
    REPORT_FREQUENCY rf = REPORT_FREQUENCY::UPTODATE;
    long reportTime = 1l;

    ASSERT_NE(cacheHandler->addSubscriber(cb, rf, reportTime), 0);
}

TEST_F(DataCacheTest, deleteSubsciber_normalCase)
{

    mocks.ExpectCall(pResource.get(), PrimitiveResource::requestGet);
    mocks.ExpectCall(pResource.get(), PrimitiveResource::isObservable).Return(true);
    mocks.ExpectCall(pResource.get(), PrimitiveResource::requestObserve);
    mocks.OnCall(pResource.get(), PrimitiveResource::cancelObserve);

    cacheHandler->initializeDataCache(pResource);

    REPORT_FREQUENCY rf = REPORT_FREQUENCY::UPTODATE;
    long reportTime = 1l;

    id = cacheHandler->addSubscriber(cb, rf, reportTime);

    ASSERT_NE(cacheHandler->deleteSubscriber(id), 0);
}

TEST_F(DataCacheTest, getCacheState_normalCase)
{

    mocks.ExpectCall(pResource.get(), PrimitiveResource::requestGet);
    mocks.ExpectCall(pResource.get(), PrimitiveResource::isObservable).Return(true);
    mocks.ExpectCall(pResource.get(), PrimitiveResource::requestObserve);
    mocks.OnCall(pResource.get(), PrimitiveResource::cancelObserve);

    cacheHandler->initializeDataCache(pResource);

    ASSERT_EQ(cacheHandler->getCacheState(), CACHE_STATE::READY_YET);
}

TEST_F(DataCacheTest, getCachedData_normalCase)
{

    mocks.ExpectCall(pResource.get(), PrimitiveResource::requestGet);
    mocks.ExpectCall(pResource.get(), PrimitiveResource::isObservable).Return(true);
    mocks.ExpectCall(pResource.get(), PrimitiveResource::requestObserve);
    mocks.OnCall(pResource.get(), PrimitiveResource::cancelObserve);

    cacheHandler->initializeDataCache(pResource);

    ASSERT_EQ(cacheHandler->getCachedData(), RCSResourceAttributes());
}

TEST_F(DataCacheTest, getPrimitiveResource_normalCase)
{

    mocks.ExpectCall(pResource.get(), PrimitiveResource::requestGet);
    mocks.ExpectCall(pResource.get(), PrimitiveResource::isObservable).Return(true);
    mocks.ExpectCall(pResource.get(), PrimitiveResource::requestObserve);
    mocks.OnCall(pResource.get(), PrimitiveResource::cancelObserve);

    cacheHandler->initializeDataCache(pResource);

    ASSERT_EQ(cacheHandler->getPrimitiveResource(), pResource);
}

TEST_F(DataCacheTest, requestGet_normalCase)
{

    mocks.ExpectCall(pResource.get(), PrimitiveResource::requestGet);
    mocks.ExpectCall(pResource.get(), PrimitiveResource::isObservable).Return(true);
    mocks.ExpectCall(pResource.get(), PrimitiveResource::requestObserve);
    mocks.ExpectCall(pResource.get(), PrimitiveResource::requestGet);
    mocks.OnCall(pResource.get(), PrimitiveResource::cancelObserve);

    cacheHandler->initializeDataCache(pResource);

    cacheHandler->requestGet();
}

TEST_F(DataCacheTest, isEmptySubscriber_normalCase)
{

    mocks.ExpectCall(pResource.get(), PrimitiveResource::requestGet);
    mocks.ExpectCall(pResource.get(), PrimitiveResource::isObservable).Return(true);
    mocks.ExpectCall(pResource.get(), PrimitiveResource::requestObserve);
    mocks.OnCall(pResource.get(), PrimitiveResource::cancelObserve);

    cacheHandler->initializeDataCache(pResource);

    ASSERT_EQ(cacheHandler->isEmptySubscriber(), true);
}

TEST_F(DataCacheTest, requestGet_normalCasetest)
{

    mocks.ExpectCall(pResource.get(), PrimitiveResource::requestGet);
    mocks.OnCall(pResource.get(), PrimitiveResource::isObservable).Return(true);
    mocks.ExpectCall(pResource.get(), PrimitiveResource::requestObserve);
    mocks.OnCall(pResource.get(), PrimitiveResource::requestGet).Do(
        [](GetCallback callback)
    {
        std::cout << "HelloWorld" << std::endl;
        OIC::Service::HeaderOptions hos;

        OIC::Service::RCSResourceAttributes attr;
        //attr = mocks.Mcok< OIC::Service::RCSResourceAttributes >();
        OIC::Service::ResponseStatement rep(attr);
        //rep = mocks.Mock< OIC::Service::ResponseStatement >(attr);
        //fakeResource = mocks.Mock< FakeOCResource >();
        //mocks.OnCallFunc(RCSResourceAttributes::empty()).Return(false);
        callback(hos, rep, OC_STACK_OK);
        return;
    }
    );
    mocks.OnCall(pResource.get(), PrimitiveResource::cancelObserve);

    cacheHandler->initializeDataCache(pResource);

    cacheHandler->requestGet();
}