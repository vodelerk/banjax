#ifndef PROCESSOR_H
#define PROCESSOR_H




#include "utils/concurrent_fifo.h"
#include "utils/svm/svm.h"
#include "external_action.h"


struct LogEntry;
class BotBangerAggregator;
class BotBangerEventListener;
class HostHitMissAggregator;
class HostHitMissEventListener;

using namespace std;

/* event listener interface for LogEntryProcessor
 * use AddLogEntry to
 */
class LogEntryProcessorEventListener
{
public:
	virtual void OnLogEntryStart(LogEntry *le)=0; // fires before a LogEntry is processed
	virtual void OnLogEntryEnd(LogEntry *le,string &output,vector<externalAction> &actionList)=0; // fires when a LogEntry has been processed
	virtual bool IsOwnedByProcessor() {return true;} // is this listener owned by the processor and should it be deleted
	virtual ~LogEntryProcessorEventListener() {;}
};

/* Main processor for LogEntries
 */
class LogEntryProcessor
{
	BotBangerAggregator *_bbag; // BotBanger
	HostHitMissAggregator *_hhmag; // HostHitMiss

	volatile bool _running; // true if started
	bool _async; // true if operating in async mode, seperate thread and a fifo queue

	ConcurrentFifo<char> _ackQueue; // ack queue, this is a synchronization queue
	ConcurrentFifo<FifoMessage *> _logEntryQueue; // log entry queue

	pthread_t _processorThreadId; // thread on which the processor runs
	string _output; // buffer for logging output
	vector<externalAction> _actionList; // actions generated by the aggregators (HostHitMiss & BotBanger)
	vector<LogEntryProcessorEventListener *> _eventListeners; // eventlisteners, owned

	/* processor startup thread, takes a pointer to LogEntryProcessor,
	 * calls innerProcessorThread
	 */
	static void * processorThread(void *arg);
	void *innerProcesserThread(); // processor thread which aggregates LogEntries
	bool AggregrateLogEntry(LogEntry *le); // process LogEntry


	bool SendStop(); // Send a stop command (1 byte instead of a LogEntry) to the _logEntryQueue
	bool SendLogEntry(LogEntry *le); // Send a LogEntry to the _logEntryQueue, is copied
	bool ReceiveLogEntry(LogEntry *le);	// Receive a LogEntry, return false if a stop command is received


	bool SendAck(); // send ack to the control queue
	bool WaitForControlAck(); // wait for ack on the control queue

	void Cleanup(); // clean up


	BotBangerAggregator *newBotbangerAggregator(int maxEntries=50000); // create a BotbangerAggregator with all the features registered

protected:


public:
	LogEntryProcessor();
	~LogEntryProcessor();

	bool AddLogEntry(LogEntry *le); // add a log entry for processing, depending on the mode (async) this will block or not
	void HitMissSetConfig(int period,int range); // set the period and range used by HostHitMiss
	void BotBangerSetConfig(int maxEntries); // set the maximum number of entries (ip addresses)
	void DumpPredictedMemoryUsage(); // Dump predicted memory usage depending on configuration to stdout
	bool Start(bool async=true); // start processing, async is true starts a seperate processing thread
	void Stop(bool asap=false); // stop processing, asap is true kills LogEntries in transit
	void RegisterEventListener(BotBangerEventListener *l); // register listener
	void RegisterEventListener(HostHitMissEventListener *l); // register listener
	void RegisterEventListener(LogEntryProcessorEventListener *l); // register listener


	friend class LogEntryProcessorConfig;
};

#endif
