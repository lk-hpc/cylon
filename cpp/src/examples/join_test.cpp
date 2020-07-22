/*
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <glog/logging.h>
#include <net/mpi/mpi_communicator.hpp>
#include <ctx/cylon_context.hpp>
#include <table.hpp>
#include <chrono>

#include "test_utils.hpp"

using namespace cylon;

int main(int argc, char *argv[]) {

  Status status;
  auto start_start = std::chrono::steady_clock::now();
  auto mpi_config = new cylon::net::MPIConfig();
  auto ctx = cylon::CylonContext::InitDistributed(mpi_config);

  int rank = ctx->GetRank();
  int world_sz = ctx->GetWorldSize();
  std::string path1 = "/tmp/cylon/input/csv1_" + std::to_string(rank) + ".csv";
  std::string path2 = "/tmp/cylon/input/csv2_" + std::to_string(rank) + ".csv";
  std::string out_path = "/tmp/cylon/output/join_" + std::to_string(world_sz) + "_" + std::to_string(rank) + ".csv";

  std::shared_ptr<cylon::Table> table1, table2, joined_expected, joined, verification;

  auto read_options = cylon::io::config::CSVReadOptions().UseThreads(false);

  status = cylon::Table::FromCSV(ctx,
                                 std::vector<std::string>{path1, path2, out_path},
                                 std::vector<std::shared_ptr<Table> *>{&table1, &table2, &joined_expected},
                                 read_options);

  if (!status.is_ok()) {
    LOG(INFO) << "Table reading failed " << status.get_msg();
    ctx->Finalize();
    return 1;
  }

  auto read_end_time = std::chrono::steady_clock::now();

  LOG(INFO) << "Read tables in "
            << std::chrono::duration_cast<std::chrono::milliseconds>(read_end_time - start_start).count() << "[ms]";

  status = table1->DistributedJoin(table2,
                                   cylon::join::config::JoinConfig::InnerJoin(0, 0), &joined);
  if (!status.is_ok()) {
    LOG(INFO) << "Table join failed ";
    ctx->Finalize();
    return 1;
  }
  auto join_end_time = std::chrono::steady_clock::now();

  LOG(INFO) << "First table had : " << table1->Rows() << " and Second table had : "
            << table2->Rows() << ", Joined has : " << joined->Rows();
  LOG(INFO) << "Join done in "
            << std::chrono::duration_cast<std::chrono::milliseconds>(join_end_time - read_end_time).count() << "[ms]";

//  joined->WriteCSV(out_path);
//  ctx->Finalize();
//  return 0;

  return test::Verify(ctx, joined, joined_expected);
}
