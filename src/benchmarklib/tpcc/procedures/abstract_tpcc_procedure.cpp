#include "abstract_tpcc_procedure.hpp"

#include "concurrency/transaction_manager.hpp"
#include "sql/sql_pipeline_builder.hpp"

namespace opossum {

AbstractTpccProcedure::AbstractTpccProcedure(BenchmarkSQLExecutor sql_executor) : _sql_executor(sql_executor) {
  _sql_executor.transaction_context = TransactionManager::get().new_transaction_context();
}

thread_local std::minstd_rand AbstractTpccProcedure::_random_engine = std::minstd_rand{42};
thread_local TpccRandomGenerator AbstractTpccProcedure::_tpcc_random_generator = TpccRandomGenerator{42};

}