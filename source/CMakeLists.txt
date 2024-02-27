#set(CMAKE_CXX_COMPILER             "/usr/bin/clang++")
#set(CMAKE_CXX_FLAGS "-std=c++11 -Wall -Wextra -Wno-missing-braces -pthread -g  -lboost_thread-mt")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wno-missing-braces -Wextra")
set(CMAKE_CXX_STANDARD 20)

find_package(Threads REQUIRED)

if(Boost_FOUND)
   #include_directories(${Boost_INCLUDE_DIRS})
   #include_directories(${CMAKE_SOURCE_DIR}/source)
   aux_source_directory(. SRC_LIST)
   aux_source_directory(./data/attribute SRC_LIST)
   aux_source_directory(./data/dictionary SRC_LIST)
   aux_source_directory(./data/dataset SRC_LIST)
   aux_source_directory(./filesystem SRC_LIST)
   aux_source_directory(./network/upperlayer SRC_LIST)
   aux_source_directory(./network/dimse SRC_LIST)
   aux_source_directory(./network SRC_LIST)
   aux_source_directory(./serviceclass SRC_LIST)
   aux_source_directory(./infrastructure SRC_LIST)
   aux_source_directory(./util SRC_LIST)
   add_library(libdicompp ${SRC_LIST}
       network/connection.hpp
       infrastructure/asio_tcp_connection.hpp infrastructure/asio_tcp_connection_manager.hpp
       filesystem/dicomfile.hpp
       data/dictionary/commanddictionary.hpp data/dictionary/datadictionary.hpp data/dictionary/dictionary.hpp data/dictionary/dictionary_dyn.hpp data/dictionary/dictionary_entry.hpp
       data/dataset/dataset_iterator.hpp data/dataset/datasets.hpp data/dataset/transfer_processor.hpp
       data/attribute/attribute.hpp data/attribute/attribute_field_coder.hpp data/attribute/base_types.hpp data/attribute/constants.hpp data/attribute/encapsulated.hpp data/attribute/tag.hpp data/attribute/vmtype.hpp
       network/dimse/association_definition.hpp network/dimse/dimse_pm.hpp network/dimse/response.hpp network/dimse/sop_class.hpp
       network/upperlayer/upperlayer.hpp network/upperlayer/upperlayer_connection.hpp network/upperlayer/upperlayer_properties.hpp network/upperlayer/upperlayer_statemachine.hpp
       network/upperlayer/upperlayer.hpp network/upperlayer/upperlayer_connection.hpp network/upperlayer/upperlayer_properties.hpp network/upperlayer/upperlayer_statemachine.hpp
       util/channel_sev_logger.hpp util/uid.hpp
       serviceclass/find_scu.hpp serviceclass/queryretrieve_scp.hpp serviceclass/serviceclass.hpp serviceclass/storage_scp.hpp serviceclass/storage_scu.hpp serviceclass/worklist_scp.hpp
       network/upperlayer/Connection.hpp
       infrastructure/dicomconnection.h
       data/dataset/tools.h)
   target_include_directories(libdicompp
       PUBLIC $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include> $<INSTALL_INTERFACE:include>)
   target_include_directories(libdicompp PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} PUBLIC ${Boost_INCLUDE_DIRS})
   target_compile_definitions(libdicompp PUBLIC BOOST_ASIO_HAS_STD_CHRONO)
   target_compile_features(libdicompp PUBLIC cxx_std_11)
   target_link_libraries(libdicompp ${Boost_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})
endif()

