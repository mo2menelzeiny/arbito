project(Disruptor)
cmake_minimum_required(VERSION 2.6)

set(DISRUPTOR_VERSION_MAJOR "1")
set(DISRUPTOR_VERSION_MINOR "0")
set(DISRUPTOR_VERSION_PATCH "0")
set(DISRUPTOR_VERSION "${DISRUPTOR_VERSION_MAJOR}.${DISRUPTOR_VERSION_MINOR}.${DISRUPTOR_VERSION_PATCH}")

# enable gcc specific stuff
if(CMAKE_COMPILER_IS_GNUCXX)
    set(COMPILER_FLAGS "-std=c++14")
    set(WARNING_FLAGS "-W -Wall")

    if(CMAKE_BUILD_TYPE STREQUAL "debug")
        message(STATUS "Enabling debugging symbols")
        set(COMPILER_FLAGS "${COMPILER_FLAGS} -g")
    else()
        set(COMPILER_FLAGS "${COMPILER_FLAGS} -O2")
    endif()

    if(CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64")
        add_definitions(-fPIC)
    endif(CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64")

elseif (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    set(COMPILER_FLAGS "-std=c++14")
    set(WARNING_FLAGS "-W -Wall")

    if(CMAKE_BUILD_TYPE STREQUAL "debug")
        message(STATUS "Enabling debugging symbols")
        set(COMPILER_FLAGS "${COMPILER_FLAGS} -g")
    else()
        set(COMPILER_FLAGS "${COMPILER_FLAGS} -O2")
    endif()

    if(CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64")
        add_definitions(-fPIC)
    endif(CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64")

endif()

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${COMPILER_FLAGS} ${WARNING_FLAGS}")

find_package(Boost COMPONENTS system)
if(Boost_FOUND)
    include_directories(${Boost_INCLUDE_DIRS})
    link_directories(${Boost_LIBRARY_DIRS})
endif()

include_directories("..")


set(Disruptor_sources

    BasicExecutor.cpp
    BlockingWaitStrategy.cpp
    BusySpinWaitStrategy.cpp
    FixedSequenceGroup.cpp
    ProcessingSequenceBarrier.cpp
    ProducerType.cpp
    RoundRobinThreadAffinedTaskScheduler.cpp
    Sequence.cpp
    SequenceGroups.cpp
    SleepingWaitStrategy.cpp
    SpinWait.cpp
    SpinWaitWaitStrategy.cpp
    stdafx.cpp
    ThreadPerTaskScheduler.cpp
    ThreadHelper_Linux.cpp
    ThreadHelper_Windows.cpp
    TimeoutBlockingWaitStrategy.cpp
    TypeInfo.cpp
    Util.cpp
    YieldingWaitStrategy.cpp
)

set(Disruptor_headers

    AggregateEventHandler.h
    AlertException.h
    ArgumentException.h
    ArgumentNullException.h
    ArgumentOutOfRangeException.h
    BasicExecutor.h
    BatchEventProcessor.h
    BlockingQueue.h
    BlockingWaitStrategy.h
    BuildConfig.h
    BusySpinWaitStrategy.h
    ClockConfig.h
    ConsumerRepository.h
    Disruptor.h
    EventHandlerGroup.h
    EventPoller.h
    EventProcessorInfo.h
    ExceptionBase.h
    ExceptionHandlerSetting.h
    ExceptionHandlerWrapper.h
    FatalException.h
    FatalExceptionHandler.h
    FixedSequenceGroup.h
    IConsumerInfo.h
    ICursored.h
    IDataProvider.h
    IEventHandler.h
    IEventProcessor.h
    IEventProcessorFactory.h
    IEventReleaseAware.h
    IEventReleaser.h
    IEventSequencer.h
    IEventTranslator.h
    IEventTranslatorVararg.h
    IExceptionHandler.h
    IExecutor.h
    IgnoreExceptionHandler.h
    IHighestPublishedSequenceProvider.h
    ILifecycleAware.h
    InsufficientCapacityException.h
    InvalidOperationException.h
    ISequence.h
    ISequenceBarrier.h
    ISequenced.h
    ISequencer.h
    ISequenceReportingEventHandler.h
    ITaskScheduler.h
    ITimeoutHandler.h
    IWaitStrategy.h
    IWorkHandler.h
    MultiProducerSequencer.h
    NoOpEventProcessor.h
    NotSupportedException.h
    Pragmas.h
    ProcessingSequenceBarrier.h
    ProducerType.h
    RingBuffer.h
    RoundRobinThreadAffinedTaskScheduler.h
    Sequence.h
    SequenceGroups.h
    Sequencer.h
    SingleProducerSequencer.h
    SleepingWaitStrategy.h
    SpinWait.h
    SpinWaitWaitStrategy.h
    ThreadPerTaskScheduler.h
    ThreadHelper.h
    TimeoutBlockingWaitStrategy.h
    TimeoutException.h
    TypeInfo.h
    Util.h
    WorkerPool.h
    WorkerPoolInfo.h
    WorkProcessor.h
    YieldingWaitStrategy.h
)

add_library(DisruptorShared SHARED ${Disruptor_sources})
set_target_properties(DisruptorShared PROPERTIES OUTPUT_NAME Disruptor)
set_target_properties(DisruptorShared PROPERTIES VERSION ${DISRUPTOR_VERSION})
set_target_properties(DisruptorShared PROPERTIES SOVERSION ${DISRUPTOR_VERSION_MAJOR})

add_library(DisruptorStatic STATIC ${Disruptor_sources})
set_target_properties(DisruptorStatic PROPERTIES OUTPUT_NAME Disruptor)

install(FILES ${Disruptor_headers} DESTINATION include/Disruptor)

install(TARGETS DisruptorShared DisruptorStatic
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)
