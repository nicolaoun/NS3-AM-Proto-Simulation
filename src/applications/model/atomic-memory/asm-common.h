#ifndef AM_COMMON_H
#define AM_COMMON_H

#include <sstream>

// Process Types
enum ProcessType{
    READER,
    WRITER,
};

// Statuses
enum Status{
    IDLE,
    PHASE1,
    PHASE2,
};

// Message Types
enum MessageType{
    WRITE,
    READ,
    INFORM,
    WRITEACK,
    READACK,
    INFORMACK,
    TERMINATE,
    CTRL_FSIZE,
    CTRL_PKT,
	READRELAY,
	DISCOVER,
	DISCOVERACK
};

// Log message level
enum logLevel_t{
	INFO,
	DEBUG
};

class AsmCommon
{
public:
	static void Reset(std::stringstream& s)
	{
		s.str("");
		s.clear();
	}
};

#endif
