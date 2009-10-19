
#include <deque>
#include <vector>
#include <map>
#include <boost/program_options.hpp>
#include <boost/foreach.hpp>
#include <boost/scoped_ptr.hpp>
#include "azlib/util/Assert.h"
#include "azlib/program.h" /* main() */
#include "mxcontrol-commands/TasksHolder.h"

using namespace std;
using namespace mxcontrol;

int AzoukMain(int argc, char** argv) {

    Assert(argc > 0);
    std::vector<std::string> args(argv + 1, argv + argc);

    //TasksHolder tasks_holder;
    tasks_holder()
	    .set_original_args(argc, argv)
	;

    // build program options
    bool show_help;
    tasks_holder().general_options.add_options()
	("help", po::bool_switch(&show_help), "produce help message")
	("logging-fd", po::value<boost::uint16_t>(), "descriptor, to which binary logging stream is sent")
	("logging-file", po::value<std::string>(), "binary logging stream file (if --logging-fd not set)")
	;

    // parse the commandline
    po::options_description cmdline_options;
    cmdline_options
	.add(tasks_holder().general_options)
	;

    po::parsed_options parsed = po::command_line_parser(argc, argv)
	.options(cmdline_options)
	.allow_unregistered()
	.run();
    po::variables_map vm;
    po::store(parsed, vm);
    po::notify(vm);

    std::vector<std::string> unrecognized_ = po::collect_unrecognized(parsed.options, po::include_positional);
    std::deque<std::string> unrecognized(unrecognized_.begin(), unrecognized_.end());
    std::vector<std::string>().swap(unrecognized_); // free all memory

    // see what're the results of parsing
    if (!unrecognized.size()) {
	unrecognized.push_back("help");
	show_help = false;
    }
    if (show_help/* && unrecognized[0] != "help"*/) {
	unrecognized.push_front("help");
	show_help = false;
    }

    if (!tasks_holder().is_command(unrecognized[0])) {
	cerr << argv[0] << ": unknown command: " << unrecognized[0] << "\n";

	// print what commands are available
	if (tasks_holder().tasks().size()) {
	    cerr << "Available commands:\n";
	    BOOST_FOREACH(const TasksHolder::TasksMap::value_type& te, tasks_holder().tasks())
		cerr << "\t" << te.first << "\n";
	} else {
	    cerr << "There are no available commands.\n";
	}

	return EXIT_FAILURE;
    }
    
    // initialize env
    if (vm.count("logging-fd")) {
	azlib::logging::set_logging_fd(vm["logging-fd"].as<boost::uint16_t>());
    } else if (vm.count("logging-file")) {
	azlib::logging::set_logging_file(vm["logging-file"].as<std::string>());
    }

    // run the command
    return tasks_holder().run(unrecognized);
}
