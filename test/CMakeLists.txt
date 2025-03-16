set(SRC_LIST
        test_db.cpp
)

add_executable(test_db_server ${SRC_LIST})



target_link_libraries(test_db_server hnc_db)

