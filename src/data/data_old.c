//// @file data.c
//// @brief File to handle data
////
//// Save and load data. Also calculates data trends and statistics for data.
////
//// @author Eric D. Phillips
//// @date November 26, 2015
//// @bugs No known bugs
//
//#include <pebble.h>
//#include "data_old.h"
//#include "../utility.h"
//
//// Constants
//#define DATA_HISTORY_INDEX_MAX 9
//#define PERSIST_DATA_LENGTH 256
//#define STATS_LAST_CHARGE_KEY 997
//#define STATS_RECORD_LIFE_KEY 998
//#define STATS_CHARGE_RATE_KEY 999
//#define DATA_PERSIST_KEY 1000
//
//// Main data structure
//typedef struct DataNode {
//  uint32_t  epoch     : 31;   //< The epoch timestamp for when the percentage changed in seconds
//  uint8_t   percent   : 7;    //< The battery charge percent
//  bool      charging  : 1;    //< The charging state of the battery
//  bool      plugged   : 1;    //< The plugged in state of the watch
//  struct DataNode *next;      //< The next node in the linked list
//} __attribute__((__packed__)) DataNode;
//
//// Main variables
//static DataNode *head_node = NULL; // Newest at start
//static uint16_t prv_node_count = 0; // Total number of nodes currently loaded in memory
//static int32_t prv_charge_rate, prv_last_charged, prv_record_life;
//static int32_t prv_past_run_times[DATA_HISTORY_INDEX_MAX] = {0};
//static int32_t prv_past_max_life[DATA_HISTORY_INDEX_MAX] = {0};
//static uint16_t prv_history_point_count = 0;
//
//
//////////////////////////////////////////////////////////////////////////////////////////////////////
//// Private Functions
////
//
//// Get the latest data node (returns the current battery value if no nodes)
//static DataNode prv_get_latest_node(void) {
//  // get current battery state
//  DataNode cur_node;
//  BatteryChargeState bat_state = battery_state_service_peek();
//  cur_node.epoch = time(NULL);
//  cur_node.percent = bat_state.charge_percent;
//  cur_node.charging = bat_state.is_charging;
//  cur_node.plugged = bat_state.is_plugged;
//  cur_node.next = NULL;
//  // return current state if no last node or not matching with last node
//  if (!head_node || head_node->percent != cur_node.percent ||
//    head_node->charging != cur_node.charging || head_node->plugged != cur_node.plugged) {
//    return cur_node;
//  }
//  return *head_node;
//}
//
//// Add node to start of linked list
//static void prv_list_add_node_start(DataNode *node) {
//  node->next = head_node;
//  head_node = node;
//  prv_node_count++;
//}
//
//// Add node to end of linked list
//static void prv_list_add_node_end(DataNode *node) {
//  if (!head_node) {
//    head_node = node;
//    prv_node_count++;
//    return;
//  }
//  DataNode *cur_node = head_node;
//  while (cur_node->next) {
//    cur_node = cur_node->next;
//  }
//  cur_node->next = node;
//  prv_node_count++;
//}
//
//// Get the past run times from flash data (assumes data is there)
//static void prv_load_past_run_times(void) {
//  // prep for stats
//  uint8_t run_time_index = 0;
//  int32_t tmp_charge_rate = prv_charge_rate;
//  bool last_was_charging = true;
//  int32_t tmp_last_charged = time(NULL);
//  prv_past_max_life[0] = tmp_charge_rate * (-100);
//  // prep for data
//  uint32_t persist_key = persist_read_int(DATA_PERSIST_KEY);
//  uint32_t worker_node_size = sizeof(DataNode) - sizeof(((DataNode*)0)->next);
//  DataNode tmp_node, tmp_node_new = prv_get_latest_node(); // new is in time, closer to present
//  char buff[PERSIST_DATA_LENGTH];
//  // loop over data
//  while (persist_key > DATA_PERSIST_KEY && persist_exists(persist_key)) {
//    persist_read_data(persist_key, buff, persist_get_size(persist_key));
//    for (char *ptr = buff + persist_get_size(persist_key) - worker_node_size; ptr >= buff;
//         ptr -= worker_node_size) {
//      // update data node
//      memcpy(&tmp_node, ptr, worker_node_size);
//      // do data calculations
//      if (tmp_node.charging) {
//        // check if going from not charging to charging
//        if (!last_was_charging) {
//          prv_past_run_times[run_time_index++] = tmp_last_charged - tmp_node.epoch;
//          // check if done
//          if (run_time_index >= DATA_HISTORY_INDEX_MAX) {
//            // log number of data points
//            prv_history_point_count = run_time_index;
//            return;
//          }
//          prv_past_max_life[run_time_index] = tmp_charge_rate * (-100);
//        }
//        tmp_last_charged = tmp_node.epoch;
//      } else if (!last_was_charging && tmp_node.percent > tmp_node_new.percent) {
//        // calculate past charge rates
//        tmp_charge_rate = tmp_charge_rate * 5 / 4 - ((tmp_node_new.epoch - tmp_node.epoch) /
//          (tmp_node_new.percent - tmp_node.percent)) / 4;
//      }
//      last_was_charging = tmp_node.charging;
//      tmp_node_new = tmp_node;
//    }
//    // index key
//    persist_key--;
//  }
//  // log number of data points
//  prv_history_point_count = run_time_index;
//}
//
//
//////////////////////////////////////////////////////////////////////////////////////////////////////
//// API Implementation
////
//
//// Get the number of points of history loaded (for run time and max life)
//uint16_t data_get_history_points_count(void) {
// return prv_history_point_count;
//}
//
//// Get the time the watch needs to be charged by
//int32_t data_get_charge_by_time(void) {
//  // get latest node
//  DataNode tmp_node = prv_get_latest_node();
//  // calculate end time
//  return tmp_node.epoch + tmp_node.percent * (-prv_charge_rate);
//}
//
//// Get the estimated time remaining in seconds
//int32_t data_get_life_remaining(void) {
//  // get latest node
//  DataNode tmp_node = prv_get_latest_node();
//  // calculate time remaining
//  return tmp_node.epoch + tmp_node.percent * (-prv_charge_rate) - time(NULL);
//}
//
//// Get the time the watch was last charged
//// Based off app install time if no data
//int32_t data_get_last_charge_time(void) {
//  return time(NULL) - data_get_run_time();
//}
//
//// Get a past run time by its index (0 is current, 1 is yesterday, etc)
//// Must be between 0 and DATA_HISTORY_INDEX_MAX
//int32_t data_get_past_run_time(uint16_t index) {
//  return prv_past_run_times[index];
//}
//
//// Get the record run time of the watch
//// Based off app install time if no data
//int32_t data_get_record_run_time(void) {
//  // check if current run time is greater than record
//  if (data_get_run_time() > prv_record_life) {
//    return data_get_run_time();
//  }
//  return prv_record_life;
//}
//
//// Get the current run time of the watch in seconds
//// If no charge data, returns app install time
//int32_t data_get_run_time(void) {
//  return time(NULL) - prv_last_charged;
//}
//
//// Get the current percent-per-day of battery life
//int32_t data_get_percent_per_day(void) {
//  return 10000 / (data_get_max_life() * 100 / SEC_IN_DAY);
//}
//
//// Get the current battery percentage (this is an estimate of the exact value)
//uint8_t data_get_battery_percent(void) {
//  // get latest node
//  DataNode tmp_node = prv_get_latest_node();
//  // calculate exact percent
//  int32_t percent = tmp_node.percent + (time(NULL) - tmp_node.epoch) / prv_charge_rate;
//  if (percent > tmp_node.percent) {
//    percent = tmp_node.percent;
//  } else if (percent <= tmp_node.percent - 10) {
//    percent = tmp_node.percent - 9;
//  }
//  if (percent < 0) {
//    percent = 0;
//  }
//  return percent;
//}
//
//// Get a past max life by its index (0 is current, 1 is yesterday, etc)
//// Must be between 0 and DATA_HISTORY_INDEX_MAX
//int32_t data_get_past_max_life(uint16_t index) {
//  return prv_past_max_life[index];
//}
//
//// Get the maximum battery life possible with the current discharge rate
//int32_t data_get_max_life(void) {
//  return prv_charge_rate * (-100);
//}
//
//// Get the current charge rate in seconds per percent (will always be negative since discharging)
//int32_t data_get_charge_rate(void) {
//  return prv_charge_rate;
//}
//
//// Get the current number of data points loaded in RAM
//uint16_t data_get_battery_data_point_count(void) {
//  return prv_node_count;
//}
//
//// Get data points by index with 0 being the latest point
//bool data_get_battery_data_point(uint16_t index, int32_t *epoch, uint8_t *percent) {
//  DataNode *cur_node = head_node;
//  uint16_t ii = 0;
//  while (cur_node) {
//    // check if node
//    if (index == ii) {
//      (*epoch) = cur_node->epoch;
//      (*percent) = cur_node->percent;
//      return true;
//    }
//    // index node
//    ii++;
//    cur_node = cur_node->next;
//  }
//  return false;
//}
//
//// Load the past X days of data
//void data_load_past_days(uint8_t num_days) {
//  if (!persist_exists(DATA_PERSIST_KEY)) {
//    return;
//  }
//  // unload any current data first
//  data_unload();
//  // load current charge rate estimate
//  prv_last_charged = persist_read_int(STATS_LAST_CHARGE_KEY);
//  prv_record_life = persist_read_int(STATS_RECORD_LIFE_KEY);
//  prv_charge_rate = persist_read_int(STATS_CHARGE_RATE_KEY);
//  // load past run time data
//  prv_load_past_run_times();
//  // prep for data
//  uint32_t persist_key = persist_read_int(DATA_PERSIST_KEY);
//  uint32_t worker_node_size = sizeof(DataNode) - sizeof(((DataNode*)0)->next);
//  char buff[PERSIST_DATA_LENGTH];
//  // loop over data
//  while (persist_key > DATA_PERSIST_KEY && persist_exists(persist_key)) {
//    persist_read_data(persist_key, buff, persist_get_size(persist_key));
//    for (char *ptr = buff + persist_get_size(persist_key) - worker_node_size; ptr >= buff;
//         ptr -= worker_node_size) {
//      // create new node
//      DataNode *new_node = MALLOC(sizeof(DataNode));
//      memcpy(new_node, ptr, worker_node_size);
//      new_node->next = NULL;
//      prv_list_add_node_end(new_node);
//      // check if passed the number of days
//      if (new_node->epoch < time(NULL) - num_days * SEC_IN_DAY) {
//        return;
//      }
//    }
//    // index key
//    persist_key--;
//  }
//}
//
//// Unload data and free memory
//void data_unload(void) {
//  DataNode *cur_node = head_node;
//  DataNode *tmp_node = NULL;
//  while (cur_node) {
//    // index node
//    tmp_node = cur_node;
//    cur_node = cur_node->next;
//    // destroy node
//    free(tmp_node);
//  }
//  head_node = NULL;
//  prv_node_count = 0;
//}
//
//// Print the data to the console
//void data_print_csv(void) {
//  // print header
//  app_log(APP_LOG_LEVEL_INFO, "", 0, "======================================");
//  app_log(APP_LOG_LEVEL_INFO, "", 0, "Battery+ by Ycleptic Studios");
//  app_log(APP_LOG_LEVEL_INFO, "", 0, "Raw Data Export, all times in UTC epoch format");
//  // print stats
//  app_log(APP_LOG_LEVEL_INFO, "", 0, "------------- Statistics -------------");
//  app_log(APP_LOG_LEVEL_INFO, "", 0, "Last Charged: %d", (int)prv_last_charged);
//  app_log(APP_LOG_LEVEL_INFO, "", 0, "Record Life: %d", (int)prv_record_life);
//  app_log(APP_LOG_LEVEL_INFO, "", 0, "Charge Rate: %d", (int)prv_charge_rate);
//  // print body
//  app_log(APP_LOG_LEVEL_INFO, "", 0, "------------- Raw Data -------------");
//  app_log(APP_LOG_LEVEL_INFO, "", 0, "Epoch,\t\t%%,\tCharge,\tPlugged,");
//  DataNode *cur_node = head_node;
//  while (cur_node) {
//    app_log(APP_LOG_LEVEL_INFO, "", 0, "%d,\t%d,\t%d,\t%d,", cur_node->epoch, cur_node->percent,
//      cur_node->charging, cur_node->plugged);
//    cur_node = cur_node->next;
//  }
//  app_log(APP_LOG_LEVEL_INFO, "", 0, "=====================================");
//}