
#include <google/protobuf/io/zero_copy_stream_impl.h>
#include "mxcontrol-commands/Task.h"
#include "mxcontrol-commands/TasksHolder.h"
#include "azlib/logging.h"
#include "azlib/protobuf/stream.h"
#include "build/multiplexer/Multiplexer.pb.h" /* generated */
#include "multiplexer/Client.h"

using namespace std;
using namespace azlib;
using namespace azlib::logging;
using namespace multiplexer;

namespace mxcontrol {

    class StreamLogs : public Task {
    public:
	virtual int run();
	virtual std::string short_description() const { return "stream logs from stdin to a Log Collector"; }
	virtual void print_help(std::ostream& out) {
	    out
		<< "Read a logging stream as produced by Azouk Logging Library from stdin\n"
		<< "and send collected records in chunks to a Log Collector.\n"
		<< "\n"
		<< _options_description()
		;
	}

    protected:
	virtual void _initialize_options_description(po::options_description& generic) {
	    generic.add_options()
		("chunksize", po::value(&chunksize_)->default_value(32), "how many LogEntries send at a time; 0 for unlimited")
		;
	    _add_multiplexer_client_options(generic);
	}

    private:
	int chunksize_;
    };
    REGISTER_MXCONTROL_SUBCOMMAND(streamlogs, mxcontrol::StreamLogs);

    static inline void send_logs(Client& client, LogEntriesMessage& logs) {
	MultiplexerMessage mxmsg;
	mxmsg.set_id(client.random64());
	mxmsg.set_from(client.instance_id());
	mxmsg.set_type(types::LOGS_STREAM);
	mxmsg.set_logging_method(LoggingMethod::CONSOLE);
	logs.SerializeToString(mxmsg.mutable_message());
	AZOUK_LOG(DEBUG, HIGHVERBOSITY,
		TEXT("scheduling LOGS_STREAM with " + str(logs.log_size()) +
		    " LogEntries (encoded on " + str(mxmsg.message().size()) + " b)")
	    );
	client.schedule_all(mxmsg);
	client.flush_all();
    }

    int StreamLogs::run() {
	using namespace azlib::protobuf;

	FileMessageInputStream fmis(0);
	multiplexer::LogEntriesMessage logs;

	multiplexer::Client& client = _multiplexer_client(peers::LOG_STREAMER);

	while (fmis.read(*logs.add_log())) {
	    // if chunksize_ == 0 (unlimited) this will be always false
	    if (logs.log_size() == chunksize_) {
		send_logs(client, logs);
		logs.clear_log();
	    }
	}
	logs.mutable_log()->RemoveLast();

	if (logs.log_size()) {
	    send_logs(client, logs);
	}

	client.flush_all();

	return 0;
    }


}; // namespace mxcontrol
