#include <pthread.h>
#include <sys/file.h>
#include <sys/un.h>
#include <unistd.h>
#include <algorithm>
#include <cassert>
#include <climits>
#include <cstdlib>
#include <exception>
#include <fstream>
#include <iostream>
#include <set>
#include <sstream>

#include <grpc/grpc.h>
#include <grpcpp/grpcpp.h>
#include <grpcpp/server.h>
#include <grpcpp/server_builder.h>
#include <grpcpp/server_context.h>

#include <grpcpp/ext/proto_server_reflection_plugin.h>
#include <grpcpp/health_check_service_interface.h>

#include "agl_shell.grpc.pb.h"

#include "homescreenhandler.h"
#include "AglShellManager.h"

grpc::ServerUnaryReactor *
GrpcServiceImpl::ActivateApp(grpc::CallbackServerContext *context,
			     const ::agl_shell_ipc::ActivateRequest* request,
			     google::protobuf::Empty* /*response*/)
{
	fprintf(stderr, "Calling into ActivateApp with app %s and output %s\n",
			request->app_id().c_str(),
			request->output_name().c_str());

	HomescreenHandler::Instance()->processAppStatusEvent(QString::fromStdString(request->app_id()),
							     QString::fromUtf8("started", -1));

	grpc::ServerUnaryReactor* reactor = context->DefaultReactor();
	reactor->Finish(grpc::Status::OK);
	return reactor;
}

grpc::ServerUnaryReactor *
GrpcServiceImpl::DeactivateApp(grpc::CallbackServerContext *context,
			       const ::agl_shell_ipc::DeactivateRequest* request,
			       google::protobuf::Empty* /*response*/)
{
	// FIXME, code here

	grpc::ServerUnaryReactor* reactor = context->DefaultReactor();
	reactor->Finish(grpc::Status::OK);
	return reactor;
}

grpc::ServerUnaryReactor *
GrpcServiceImpl::SetAppSplit(grpc::CallbackServerContext *context,
	    const ::agl_shell_ipc::SplitRequest* request,
	    google::protobuf::Empty* /*response*/)
{
	// FIXME, code here

	grpc::ServerUnaryReactor* reactor = context->DefaultReactor();
	reactor->Finish(grpc::Status::OK);
	return reactor;
}

grpc::ServerUnaryReactor *
GrpcServiceImpl::SetAppFloat(grpc::CallbackServerContext *context,
			     const ::agl_shell_ipc::FloatRequest* request,
			     google::protobuf::Empty* /* response */)
{
	// FIXME, code here

	grpc::ServerUnaryReactor* reactor = context->DefaultReactor();
	reactor->Finish(grpc::Status::OK);
	return reactor;
}
