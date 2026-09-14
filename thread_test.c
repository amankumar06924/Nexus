#include"my_thread.h"
#define TEST_CLIENTS 10
void *update_worker(void *arg)
{
    int client_id = *(int *)arg;

    for (int i = 0; i < 1000; i++) {
        int new_fd = 1000 + i;

        int result = update_client_fd(client_id, new_fd);

        if (result != CLIENT_OK) {
            printf("update failed: %d\n", result);
        }
    }

    return NULL;
}

void *add_remove_worker(void *arg)
{
    int thread_id = *(int *)arg;

    for (int i = 0; i < 100; i++) {
        int client_id = thread_id * 1000 + i;
        int client_fd = 2000 + client_id;

        int result = add_client(client_fd, client_id);

        if (result == CLIENT_OK) {
            int disconnect_result = disconnect_client(client_id);

            if (disconnect_result != CLIENT_OK) {
                printf("disconnect failed for client %d: %d\n",
                       client_id,
                       disconnect_result);
            }
        } else if (result != CLIENT_ERR_FULL) {
            printf("add failed for client %d: %d\n",
                   client_id,
                   result);
        }
    }

    return NULL;
}
int main(){
if(client_manager_init()!=0){printf("manager init failed\n");return 1;}
//struct data data1[TEST_CLIENTS] = {
//    {100, 1},
  //  {101, 2},
    //{102, 3},
  //  {103, 4},
  //  {104, 5},
  //  {105, 6},
  //  {106, 7},
  //  {107, 8},
  //  {108, 9},
   // {109, 10}
//};
// printf("\n--- Concurrent add/remove test ---\n");

// pthread_t add_remove_threads[5];
// int thread_ids[5];

// for (int i = 0; i < 5; i++) {
//     thread_ids[i] = i;

//     pthread_create(
//         &add_remove_threads[i],
//         NULL,
//         add_remove_worker,
//         &thread_ids[i]
//     );
// }

// for (int i = 0; i < 5; i++) {
//     pthread_join(add_remove_threads[i], NULL);
// }

// if (count_active_clients() == 0) {
//     printf("Concurrent add/remove test: PASS\n");
// } else {
//     printf("Concurrent add/remove test: FAIL\n");
//     print_clients();
// }
//pthread_t threads[TEST_CLIENTS];

//for (int i = 0; i < TEST_CLIENTS; i++) {
 //   int result = pthread_create(
 //       &threads[i],
 //       NULL,
 //       worker,
 //       &data1[i]
  //  );

   // if (result != 0) {
     //   printf("failed thread at %d\n", i);
      //  return 1;
   // }
//}
//for (int i = 0; i < TEST_CLIENTS; i++) {
//    int result = pthread_join(threads[i], NULL);
//
 //   if (result != 0) {
 //       printf("failed join at %d\n", i);
 //       return 1;
 //   }
//}

// printf("\n--- Init and destroy test ---\n");

// /* Add a client */
// int result = add_client(3000, 50);

// if (result != CLIENT_OK) {
//     printf("Initial add failed: %d\n", result);
//     return 1;
// }
// client a;
// /* Verify client exists */
// if (find_client(50,&a) == -1) {
//     printf("Client setup failed\n");
//     return 1;
// }

// /* Destroy manager */
// result = client_manager_destroy();

// if (result != CLIENT_OK) {
//     printf("client_manager_destroy: FAIL (%d)\n", result);
//     return 1;
// }

// printf("client_manager_destroy: PASS\n");

// /* Reinitialize manager */
// result = client_manager_init();

// if (result != CLIENT_OK) {
//     printf("client_manager_init after destroy: FAIL (%d)\n", result);
//     return 1;
// }

// /* Verify table is clean */
// if (count_active_clients() == 0) {
//     printf("Reinitialized table empty: PASS\n");
// } else {
//     printf("Reinitialized table empty: FAIL\n");
//     print_clients();
//     return 1;
// }
// printf("\n--- Client snapshot test ---\n");

// client snapshot;

// int result = get_client_snapshot(0, &snapshot);

// if (result == CLIENT_OK) {
//     printf("Snapshot valid index: PASS\n");
// } else {
//     printf("Snapshot valid index: FAIL\n");
//     return 1;
// }

// result = get_client_snapshot(-1, &snapshot);

// if (result == CLIENT_ERR_INVALID) {
//     printf("Snapshot invalid index: PASS\n");
// } else {
//     printf("Snapshot invalid index: FAIL\n");
//     return 1;
// }

// result = get_client_snapshot(MAX_CLIENTS, &snapshot);

// if (result == CLIENT_ERR_INVALID) {
//     printf("Snapshot out-of-range index: PASS\n");
// } else {
//     printf("Snapshot out-of-range index: FAIL\n");
//     return 1;
// }

// result = get_client_snapshot(0, NULL);

// if (result == CLIENT_ERR_INVALID) {
//     printf("Snapshot NULL output: PASS\n");
// } else {
//     printf("Snapshot NULL output: FAIL\n");
//     return 1;
// }
// printf("\n--- connect_client API test ---\n");

// int result = connect_client(4000, 77);

// if (result == CLIENT_OK) {
//     printf("connect_client success: PASS\n");
// } else {
//     printf("connect_client success: FAIL (%d)\n", result);
//     return 1;
// }
// client client_info;
// int index = find_connected_client(77, &client_info);

// if (index != -1) {
//     printf("Connected client lookup: PASS\n");
// } else {
//     printf("Connected client lookup: FAIL\n");
//     return 1;
// }

// result = connect_client(4001, 77);

// if (result == CLIENT_ERR_DUPLICATE) {
//     printf("Duplicate connect: PASS\n");
// } else {
//     printf("Duplicate connect: FAIL (%d)\n", result);
//     return 1;
// }

// result = disconnect_client(77);

// if (result == CLIENT_OK) {
//     printf("Disconnect connected client: PASS\n");
// } else {
//     printf("Disconnect connected client: FAIL (%d)\n", result);
//     return 1;
// }
// 
// 

// printf("\n--- Client statistics test ---\n");

// int result;

// result = connect_client(10000, 400);

// if (result != CLIENT_OK) {
//     printf("Client 400 connect: FAIL\n");
//     return 1;
// }

// result = connect_client(10001, 401);

// if (result != CLIENT_OK) {
//     printf("Client 401 connect: FAIL\n");
//     return 1;
// }

// int connected = count_connected_clients();
// int closing = count_closing_clients();
// int free_slots = count_free_slots();

// if (connected == 2) {
//     printf("Connected count: PASS\n");
// } else {
//     printf("Connected count: FAIL (%d)\n", connected);
//     return 1;
// }

// if (closing == 0) {
//     printf("Closing count: PASS\n");
// } else {
//     printf("Closing count: FAIL (%d)\n", closing);
//     return 1;
// }

// if (free_slots == MAX_CLIENTS - 2) {
//     printf("Free slot count: PASS\n");
// } else {
//     printf("Free slot count: FAIL (%d)\n", free_slots);
//     return 1;
// }

// disconnect_client(400);
// disconnect_client(401);

// if (count_connected_clients() == 0 &&
//     count_free_slots() == MAX_CLIENTS) {
//     printf("Statistics after cleanup: PASS\n");
// } else {
//     printf("Statistics after cleanup: FAIL\n");
//     return 1;
// }
// printf("\n--- Client manager validation test ---\n");

// int result = client_manager_reset();

// if (result == CLIENT_OK) {
//     printf("Reset before validation: PASS\n");
// } else {
//     printf("Reset before validation: FAIL (%d)\n", result);
//     return 1;
// }

// result = client_manager_validate();

// if (result == CLIENT_OK) {
//     printf("Empty manager valid: PASS\n");
// } else {
//     printf("Empty manager valid: FAIL (%d)\n", result);
//     return 1;
// }
// client client_info;
// result = connect_client(12000, 600);

// if (result == CLIENT_OK) {
//     printf("Client connect: PASS\n");
// } else {
//     printf("Client connect: FAIL (%d)\n", result);
//     return 1;
// }

// result = client_manager_validate();

// if (result == CLIENT_OK) {
//     printf("Valid active client: PASS\n");
// } else {
//     printf("Valid active client: FAIL (%d)\n", result);
//     return 1;
// }

// result = disconnect_client(600);

// if (result == CLIENT_OK) {
//     printf("Client cleanup: PASS\n");
// } else {
//     printf("Client cleanup: FAIL (%d)\n", result);
//     return 1;
// }

// result = client_manager_validate();

// if (result == CLIENT_OK) {
//     printf("Valid after cleanup: PASS\n");
// } else {
//     printf("Valid after cleanup: FAIL (%d)\n", result);
//     return 1;
// }
client_manager_reset();

assert(client_manager_add(10, 100) == CLIENT_OK);
assert(client_manager_add(20, 200) == CLIENT_OK);
assert(client_manager_add(30, 300) == CLIENT_OK);

assert(client_manager_connect(10, 100) == CLIENT_OK);
assert(client_manager_connect(20, 200) == CLIENT_OK);

int closing = client_manager_mark_all_closing();

printf("Marked closing: %d\n", closing);
assert(closing == 2);

int disconnected = client_manager_disconnect_all();

printf("Disconnected all: %d\n", disconnected);
assert(disconnected == 2);

assert(client_manager_validate() == CLIENT_OK);
// printf("\n--- client_manager_reset test ---\n");

// int result;

// /* Add clients */
// result = connect_client(11000, 500);

// if (result == CLIENT_OK) {
//     printf("Client 500 connect: PASS\n");
// } else {
//     printf("Client 500 connect: FAIL (%d)\n", result);
//     return 1;
// }

// result = connect_client(11001, 501);

// if (result == CLIENT_OK) {
//     printf("Client 501 connect: PASS\n");
// } else {
//     printf("Client 501 connect: FAIL (%d)\n", result);
//     return 1;
// }

// /* Reset manager */
// result = client_manager_reset();

// if (result == CLIENT_OK) {
//     printf("Manager reset: PASS\n");
// } else {
//     printf("Manager reset: FAIL (%d)\n", result);
//     return 1;
// }

// /* Verify all slots are free */
// if (count_connected_clients() == 0 &&
//     count_closing_clients() == 0 &&
//     count_free_slots() == MAX_CLIENTS) {
//     printf("All slots free: PASS\n");
// } else {
//     printf("All slots free: FAIL\n");
//     return 1;
// }
// client client_info;
// /* Verify clients no longer exist */
// if (find_client(500, &client_info) == -1 &&
//     find_client(501, &client_info) == -1) {
//     printf("Clients removed: PASS\n");
// } else {
//     printf("Clients removed: FAIL\n");
//     return 1;
// }
// printf("\n--- Concurrent add/remove test ---\n");

// pthread_t add_remove_threads[5];
// int thread_ids[5];

// for (int i = 0; i < 5; i++) {
//     thread_ids[i] = i;

//     pthread_create(
//         &add_remove_threads[i],
//         NULL,
//         add_remove_worker,
//         &thread_ids[i]
//     );
// }

// for (int i = 0; i < 5; i++) {
//     pthread_join(add_remove_threads[i], NULL);
// }

// if (count_active_clients() == 0) {
//     printf("Concurrent add/remove test: PASS\n");
// } else {
//     printf("Concurrent add/remove test: FAIL\n");
//     print_clients();
// }
// printf("\n--- Client state transition test ---\n");

// int result = mark_client_closing(2);

// if (result != CLIENT_OK) {
//     printf("mark_client_closing failed: %d\n", result);
//     return 1;
// }
// client client_info;
// int connected_index = find_connected_client(2, &client_info);

// if (connected_index == -1) {
//     printf("find_connected_client after closing: PASS\n");
// } else {
//     printf("find_connected_client after closing: FAIL\n");
//     return 1;
// }

// int active_index = find_client(2, &client_info);

// if (active_index != -1) {
//     printf("find_client after closing: PASS\n");
// } else {
//     printf("find_client after closing: FAIL\n");
//     return 1;
// }

// result = disconnect_client(2);

// if (result == CLIENT_OK && find_client(2, &client_info) == -1) {
//     printf("disconnect after closing: PASS\n");
// } else {
//     printf("disconnect after closing: FAIL\n");
//     return 1;
// }
// printf("\n--- Concurrent update test ---\n");

// pthread_t update_threads[5];
// int update_client_id = 2;

// for (int i = 0; i < 5; i++) {
//     int result = pthread_create(
//         &update_threads[i],
//         NULL,
//         update_worker,
//         &update_client_id
//     );

//     if (result != 0) {
//         printf("Failed to create update thread %d\n", i);
//         return 1;
//     }
// }

// for (int i = 0; i < 5; i++) {
//     int result = pthread_join(update_threads[i], NULL);

//     if (result != 0) {
//         printf("Failed to join update thread %d\n", i);
//         return 1;
//     }
// }

// int final_fd;

// if (get_client_fd(2, &final_fd) == CLIENT_OK) {
//     printf("Concurrent update test: PASS\n");
//     printf("Final FD of client 2: %d\n", final_fd);
// } else {
//     printf("Concurrent update test: FAIL\n");
// }
// // printf("\n--- Update client FD test ---\n");

// // int result;
// // int updated_fd;

// // /*
// //  * Existing client update.
// //  */
// // result = update_client_fd(2, 777);

// // if (result == CLIENT_OK) {
// //     printf("Update existing client FD: PASS\n");
// // } else {
// //     printf("Update existing client FD: FAIL (%d)\n", result);
// // }

// // /*
// //  * Verify updated FD.
// //  */
// // result = get_client_fd(2, &updated_fd);

// // if (result == CLIENT_OK && updated_fd == 777) {
// //     printf("Verify updated FD: PASS\n");
// // } else {
// //     printf("Verify updated FD: FAIL\n");
// // }

// // /*
// //  * Missing client.
// //  */
// // result = update_client_fd(9999, 888);

// // if (result == CLIENT_ERR_NOT_FOUND) {
// //     printf("Update missing client: PASS\n");
// // } else {
// //     printf("Update missing client: FAIL (%d)\n", result);
// // }

// // /*
// //  * Invalid input.
// //  */
// // result = update_client_fd(-1, 888);

// // if (result == CLIENT_ERR_INVALID) {
// //     printf("Update invalid client: PASS\n");
// // } else {
// //     printf("Update invalid client: FAIL (%d)\n", result);
// // }
// // // printf("\n--- Error code test ---\n");

// // // int result;

// // // /* Invalid client */
// // // result = add_client(-10, 500);

// // // if (result == CLIENT_ERR_INVALID) {
// //     printf("Invalid client test: PASS\n");
// // } else {
// //     printf("Invalid client test: FAIL (%d)\n", result);
// // }

// // /* Existing client ID */
// // result = add_client(500, 2);

// // if (result == CLIENT_ERR_DUPLICATE) {
// //     printf("Duplicate error code test: PASS\n");
// // } else {
// //     printf("Duplicate error code test: FAIL (%d)\n", result);
// // }

// // /* Missing client */
// // result = remove_client(9999);

// // if (result == CLIENT_ERR_NOT_FOUND) {
// //     printf("Not-found error code test: PASS\n");
// // } else {
// //     printf("Not-found error code test: FAIL (%d)\n", result);
// // }


// // // printf("\n--- Real duplicate test ---\n");

// // // /*
// // //  * Make one slot free.
// // //  */
// // // if (disconnect_client(1) == 0) {
// // //     printf("Client 1 disconnected: PASS\n");
// // // } else {
// // //     printf("Client 1 disconnected: FAIL\n");
// // // }

// // // /*
// // //  * Now slot is free, but client_id 2 already exists.
// // //  */
// // // if (add_client(555, 2) == -1) {
// // //     printf("Duplicate client ID test: PASS\n");
// // // } else {
// // //     printf("Duplicate client ID test: FAIL\n");
// // // }

// // /*
// //  * New unique ID should work.
// //  */
// // if (add_client(555, 999) == 0) {
// //     printf("Unique client ID test: PASS\n");
// // } else {
// //     printf("Unique client ID test: FAIL\n");
// // }

// print_clients();
// // printf("\n--- Full capacity test ---\n");

// // struct data extra = {
// //     .client_fd = 999,
// //     .client_id = 999
// // };

// // if (add_client(extra.client_fd, extra.client_id) == -1) {
// //     printf("Full capacity test: PASS\n");
// // } else {
// //     printf("Full capacity test: FAIL\n");
// // }

// // printf("\n--- Duplicate client ID test ---\n");

// // if (add_client(555, 1) == -1) {
// //     printf("Duplicate client ID test: PASS\n");
// // } else {
// //     printf("Duplicate client ID test: FAIL\n");
// // }

// // client result;
// // int fd;

// // printf("\n--- NULL pointer tests ---\n");

// // if (find_client(1, NULL) == -1) {
// //     printf("find_client(NULL): PASS\n");
// // } else {
// //     printf("find_client(NULL): FAIL\n");
// // }

// // if (get_client_fd(1, NULL) == -1) {
// //     printf("get_client_fd(NULL): PASS\n");
// // } else {
// //     printf("get_client_fd(NULL): FAIL\n");
// }

// if (find_client(1, &result) == 0) {
//     printf("Found client: id=%d fd=%d\n",
//            result.client_id,
//            result.client_fd);
// }

// if (get_client_fd(1, &fd) == 0) {
//     printf("Client fd: %d\n", fd);
// }
if(client_manager_destroy()!=0){
printf("manager destroy failed\n");
return 1;
}
return 0;
}
