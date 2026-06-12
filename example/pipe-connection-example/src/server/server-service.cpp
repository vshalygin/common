#include "server-service.h"
#include "rpc-lib/closure-guard/closure-guard.h"
#include "rpc-lib/types/request-result.h"

namespace vshalygin::example {
    void server_service::get_data(::google::protobuf::RpcController *controller,
                                  const ::proto::client_request *request,
                                  ::proto::client_response *response,
                                  ::google::protobuf::Closure *done)
    {
        assert(!controller);
        assert(request);
        assert(response);
        assert(done);

        rpc::closure_guard guard(done);
        if(request->data() == "data1") {
            response->set_data("response to data1");
        } else if(request->data() == "data2") {
            response->set_data("response to data2");
        } else {
            response->set_data("response to unknown data");
        }
    }
}
