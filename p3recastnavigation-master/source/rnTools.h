/**
 * \file rnTools.h
 *
 * \date 2016-03-18
 * \author consultit
 */

#ifndef RNTOOLS_H_
#define RNTOOLS_H_

#ifdef _WIN32
#include <ciso646>
#define STRTOF (float)strtod
#else
#define STRTOF strtof
#endif

#include "recastnavigation_includes.h"
#include "genericAsyncTask.h"
#include "lpoint3.h"
#include "config_module.h"

//
#ifndef CPPPARSER
#include "support/NavMeshType.h"
#endif //CPPPARSER

//continue if condition is true else return a value
#define CONTINUE_IF_ELSE_R(condition, return_value) \
  { \
    if (!(condition)) { \
      return return_value; \
    } \
  }
//continue if condition is true else return (void)
#define CONTINUE_IF_ELSE_V(condition) \
  { \
    if (!(condition)) { \
      return; \
    } \
  }

/**
 * An automatic Singleton Utility.
 *
 * \note This Singleton class is based on the article "An automatic
 * Singleton Utility" by Scott Bilas in "Game Programming Gems 1" book.
 * Non multi-threaded.
 */
template<typename T> class Singleton
{
	static T* ms_Singleton;

public:
	Singleton(void)
	{
		assert(!ms_Singleton);
		unsigned long int offset = (unsigned long int) (T*) 1
				- (unsigned long int) (Singleton<T>*) (T*) 1;
		ms_Singleton = (T*) ((unsigned long int) this + offset);
	}
	~Singleton(void)
	{
		assert(ms_Singleton);
		ms_Singleton = 0;
	}
	static T& GetSingleton(void)
	{
		assert(ms_Singleton);
		return (*ms_Singleton);
	}
	static T* GetSingletonPtr(void)
	{
		return (ms_Singleton);
	}
};

template<typename T> T* Singleton<T>::ms_Singleton = 0;

/**
 * A std::pair wrapper
 */
template<typename T1, typename T2> struct Pair
{
PUBLISHED:
	Pair() :
			mPair()
	{
	}
	Pair(const T1& first, const T2& second) :
			mPair(first, second)
	{
	}
	bool operator== (const Pair &other) const
	{
		return mPair == other.mPair;
	}
	INLINE void set_first(const T1& first)
	{
		mPair.first = first;
	}
	INLINE T1 get_first() const
	{
		return mPair.first;
	}
	INLINE void set_second(const T2& second)
	{
		mPair.second = second;
	}
	INLINE T2 get_second() const
	{
		return mPair.second;
	}

public:
	T1& first()
	{
		return mPair.first;
	}
	T2& second()
	{
		return mPair.second;
	}
private:
	std::pair<T1, T2> mPair;
};

/**
 * A pair that can be used with PT/CPT (C++ only)
 */
template<typename T1, typename T2> struct PairRC: public Pair<T1, T2>,
		public ReferenceCount
{
public:
	PairRC() :
			Pair<T1, T2>()
	{
	}
	PairRC(const T1& first, const T2& second) :
			Pair<T1, T2>(first, second)
	{
	}
};

/**
 * Template struct for generic Task Function interface
 *
 * The effective Tasks are composed by a Pair of an object and
 * a method (member function) doing the effective task.
 * To register a task:
 * 1) in class A define a (pointer to) TaskData member:
 * \code
 * 	SMARTPTR(TaskInterface<A>::TaskData) myData;
 * \endcode
 * 2) and a method (that will execute the real task) with signature:
 * \code
 * 	AsyncTask::DoneStatus myTask(GenericAsyncTask* task);
 * \endcode
 * 3) in code associate to myData a new TaskData referring to this
 * class instance and myTask, then create a new GenericAsyncTask
 * referring to taskFunction and with data parameter equal to
 * myData (reinterpreted as void*):
 * \code
 * 	myData = new TaskInterface<A>::TaskData(this, &A::myTask);
 * 	AsyncTask* task = new GenericAsyncTask("my task",
 * 							&TaskInterface<A>::taskFunction,
 * 							reinterpret_cast<void*>(myData.p()));
 * \endcode
 * 4) finally register the async-task to your manager:
 * \code
 * 	pandaFramework.get_task_mgr().add(task);
 * 	\endcode
 * From now on myTask will execute the task, while being able
 * to refer directly to data members of the A class instance.
 */
template<typename A> struct TaskInterface
{
	typedef AsyncTask::DoneStatus (A::*TaskPtr)(GenericAsyncTask* taskFunction);
	typedef PairRC<A*, TaskPtr> TaskData;
	static AsyncTask::DoneStatus taskFunction(GenericAsyncTask* task,
			void * data)
	{
		TaskData* appData = reinterpret_cast<TaskData*>(data);
		return ((appData->first())->*(appData->second()))(task);
	}
};

/**
 * Throwing event data.
 *
 * Data related to throwing events by components.
 */
class ThrowEventData
{
	struct Period
	{
		Period() :
				mPeriod(1.0 / 30.0)
		{
		}
		Period(float period) :
				mPeriod(period)
		{
		}
		Period& operator =(const Period& other)
		{
			mPeriod = other.mPeriod;
			return *this;
		}
		Period& operator =(float value)
		{
			mPeriod = value;
			return *this;
		}
		operator float() const
		{
			return mPeriod;
		}
	private:
		float mPeriod;
	};
	struct Frequency
	{
		Frequency() :
				mFrequency(30.0), mPeriod(1.0 / 30.0), mEventData(NULL)
		{
		}
		Frequency(float value) :
				mFrequency(value >= 0 ? value : -value)
		{
			mFrequency <= FLT_MIN ?
					mPeriod = FLT_MAX : mPeriod = 1.0 / mFrequency;
			mEventData = NULL;
		}
		Frequency& operator =(const Frequency& other)
		{
			mFrequency = other.mFrequency;
			mPeriod = other.mPeriod;
			return *this;
		}
		Frequency& operator =(float value)
		{
			value >= 0 ? mFrequency = value : mFrequency = -value;
			mFrequency <= FLT_MIN ?
					mPeriod = FLT_MAX : mPeriod = 1.0 / mFrequency;
			if (mEventData != NULL)
			{
				mEventData->mPeriod = mPeriod;
			}
			return *this;
		}
		operator float() const
		{
			return mFrequency;
		}
		void setEventData(ThrowEventData *eventData)
		{
			mEventData = eventData;
		}
	private:
		float mFrequency;
		Period mPeriod;
		ThrowEventData *mEventData;
	};

public:
	ThrowEventData() :
			mEnable(false), mEventName(std::string("")), mThrown(false), mTimeElapsed(
					0), mCount(0), mFrequency(30.0)
	{
		mFrequency.setEventData(this);
	}
	bool mEnable;
	std::string mEventName;
	bool mThrown;
	float mTimeElapsed;
	unsigned int mCount;
	Frequency mFrequency;
	Period mPeriod;

public:
	void write_datagram(Datagram &dg) const;
	void read_datagram(DatagramIterator &scan);
};

/**
 * Declarations for parameters management.
 */
typedef std::multimap<std::string, std::string> ParameterTable;
typedef std::multimap<std::string, std::string>::iterator ParameterTableIter;
typedef std::multimap<std::string, std::string>::const_iterator ParameterTableConstIter;
typedef std::map<std::string, ParameterTable> ParameterTableMap;
typedef std::pair<std::string, std::string> ParameterNameValue;

/**
 * Template function for conversion values to string.
 */
template<typename Type> std::string str(Type value)
{
	return static_cast<ostringstream&>(ostringstream().operator <<(value)).str();
}

/**
 * Parses a string composed by substrings separated by a character
 * separator.
 * \note all blanks are erased before parsing.
 * @param srcCompoundString The source string.
 * @param separator The character separator.
 * @return The substrings vector.
 */
pvector<std::string> parseCompoundString(
		const std::string& srcCompoundString, char separator);

/**
 * \brief Into a given string, replaces any occurrence of a character with
 * another character.
 * @param source The source string.
 * @param character Character to be replaced .
 * @param replacement Replaced character.
 * @return The result string.
 */
std::string replaceCharacter(const std::string& source, int character,
		int replacement);

/**
 * \brief Into a given string, erases any occurrence of a given character.
 * @param source The source string.
 * @param character To be erased character.
 * @return The result string.
 */
string eraseCharacter(const string& source, int character);

///NavMesh settings.
struct EXPCL_PANDARN RNNavMeshSettings
{
PUBLISHED:
	RNNavMeshSettings();
#ifndef CPPPARSER
	RNNavMeshSettings(const rnsup::NavMeshSettings& settings) :
			_navMeshSettings(settings)
	{
	}
	operator rnsup::NavMeshSettings() const
	{
		return _navMeshSettings;
	}
#endif //CPPPARSER
	INLINE float get_cellSize() const;
	INLINE void set_cellSize(float value);
	INLINE float get_cellHeight() const;
	INLINE void set_cellHeight(float value);
	INLINE float get_agentHeight() const;
	INLINE void set_agentHeight(float value);
	INLINE float get_agentRadius() const;
	INLINE void set_agentRadius(float value);
	INLINE float get_agentMaxClimb() const;
	INLINE void set_agentMaxClimb(float value);
	INLINE float get_agentMaxSlope() const;
	INLINE void set_agentMaxSlope(float value);
	INLINE float get_regionMinSize() const;
	INLINE void set_regionMinSize(float value);
	INLINE float get_regionMergeSize() const;
	INLINE void set_regionMergeSize(float value);
	INLINE float get_edgeMaxLen() const;
	INLINE void set_edgeMaxLen(float value);
	INLINE float get_edgeMaxError() const;
	INLINE void set_edgeMaxError(float value);
	INLINE float get_vertsPerPoly() const;
	INLINE void set_vertsPerPoly(float value);
	INLINE float get_detailSampleDist() const;
	INLINE void set_detailSampleDist(float value);
	INLINE float get_detailSampleMaxError() const;
	INLINE void set_detailSampleMaxError(float value);
	INLINE int get_partitionType() const;
	INLINE void set_partitionType(int value);
	void output(ostream &out) const;
private:
#ifndef CPPPARSER
	rnsup::NavMeshSettings _navMeshSettings;
#endif //CPPPARSER

public:
	void write_datagram(Datagram &dg) const;
	void read_datagram(DatagramIterator &scan);
};
INLINE std::ostream &operator << (std::ostream &out, const RNNavMeshSettings & settings);

///NavMesh tile settings.
struct EXPCL_PANDARN RNNavMeshTileSettings
{
PUBLISHED:
	RNNavMeshTileSettings();
#ifndef CPPPARSER
	RNNavMeshTileSettings(const rnsup::NavMeshTileSettings& settings) :
			_navMeshTileSettings(settings)
	{
	}
	operator rnsup::NavMeshTileSettings() const
	{
		return _navMeshTileSettings;
	}
#endif //CPPPARSER
	INLINE bool get_buildAllTiles() const;
	INLINE void set_buildAllTiles(bool value);
	INLINE int get_maxTiles() const;
	INLINE void set_maxTiles(int value);
	INLINE int get_maxPolysPerTile() const;
	INLINE void set_maxPolysPerTile(int value);
	INLINE float get_tileSize() const;
	INLINE void set_tileSize(float value);
	void output(ostream &out) const;
private:
#ifndef CPPPARSER
	rnsup::NavMeshTileSettings _navMeshTileSettings;
#endif //CPPPARSER

public:
	void write_datagram(Datagram &dg) const;
	void read_datagram(DatagramIterator &scan);
};
INLINE std::ostream &operator << (std::ostream &out, const RNNavMeshTileSettings & settings);

///Convex volume settings.
struct EXPCL_PANDARN RNConvexVolumeSettings
{
PUBLISHED:
	RNConvexVolumeSettings();

	INLINE bool operator== (const RNConvexVolumeSettings &other) const;
	INLINE int get_area() const;
	INLINE void set_area(int value);
	INLINE int get_flags() const;
	INLINE void set_flags(int value);
	INLINE LPoint3f get_centroid() const;
	INLINE void set_centroid(LPoint3f value);
	INLINE int get_ref() const;
	INLINE void set_ref(int value);
	void output(ostream &out) const;
private:
	int _area;
	int _flags;
	LPoint3f _centroid;
	int _ref;

public:
	void write_datagram(Datagram &dg) const;
	void read_datagram(DatagramIterator &scan);
};
INLINE ostream &operator << (ostream &out, const RNConvexVolumeSettings & settings);

///Off mesh connection settings.
struct EXPCL_PANDARN RNOffMeshConnectionSettings
{
PUBLISHED:
	RNOffMeshConnectionSettings();

	INLINE bool operator==(
			const RNOffMeshConnectionSettings &other) const;
	INLINE float get_rad() const;
	INLINE void set_rad(float value);
	INLINE bool get_bidir() const;
	INLINE void set_bidir(bool value);
	INLINE unsigned int get_userId() const;
	INLINE void set_userId(unsigned int value);
	INLINE int get_area() const;
	INLINE void set_area(int value);
	INLINE int get_flags() const;
	INLINE void set_flags(int value);
	INLINE int get_ref() const;
	INLINE void set_ref(int value);
	void output(ostream &out) const;
private:
	float _rad;
	bool _bidir;
	unsigned int _userId;
	int _area;
	int _flags;
	int _ref;

public:
	void write_datagram(Datagram &dg) const;
	void read_datagram(DatagramIterator &scan);
};
INLINE ostream &operator << (ostream &out, const RNOffMeshConnectionSettings & settings);

///Obstacle settings.
struct EXPCL_PANDARN RNObstacleSettings
{
PUBLISHED:
	RNObstacleSettings();

	INLINE bool operator==(
			const RNObstacleSettings &other) const;
	INLINE float get_radius() const;
	INLINE void set_radius(float value);
	INLINE LVecBase3f get_dims() const;
	INLINE void set_dims(const LVecBase3f& value);
	INLINE unsigned int get_ref() const;
	INLINE void set_ref(unsigned int value);
	void output(ostream &out) const;
private:
	float _radius;
	LVecBase3f _dims;
	unsigned int _ref;

public:
	void write_datagram(Datagram &dg) const;
	void read_datagram(DatagramIterator &scan);
};
INLINE ostream &operator << (ostream &out, const RNObstacleSettings & settings);

///CrowdAgentParams
struct EXPCL_PANDARN RNCrowdAgentParams
{
PUBLISHED:
	RNCrowdAgentParams();
#ifndef CPPPARSER
	RNCrowdAgentParams(const dtCrowdAgentParams& params) :
			_dtCrowdAgentParams(params)
	{
	}
	operator dtCrowdAgentParams() const
	{
		return _dtCrowdAgentParams;
	}
#endif //CPPPARSER
	INLINE float get_radius() const;
	INLINE void set_radius(float value);
	INLINE float get_height() const;
	INLINE void set_height(float value);
	INLINE float get_maxAcceleration() const;
	INLINE void set_maxAcceleration(float value);
	INLINE float get_maxSpeed() const;
	INLINE void set_maxSpeed(float value);
	INLINE float get_collisionQueryRange() const;
	INLINE void set_collisionQueryRange(float value);
	INLINE float get_pathOptimizationRange() const;
	INLINE void set_pathOptimizationRange(float value);
	INLINE float get_separationWeight() const;
	INLINE void set_separationWeight(float value);
	INLINE unsigned char get_updateFlags() const;
	INLINE void set_updateFlags(unsigned char value);
	INLINE unsigned char get_obstacleAvoidanceType() const;
	INLINE void set_obstacleAvoidanceType(unsigned char value);
	INLINE unsigned char get_queryFilterType() const;
	INLINE void set_queryFilterType(unsigned char value);
	INLINE void* get_userData() const;
	INLINE void set_userData(void* value);
	void output(ostream &out) const;
private:
#ifndef CPPPARSER
	dtCrowdAgentParams _dtCrowdAgentParams;
#endif //CPPPARSER

public:
	void write_datagram(Datagram &dg) const;
	void read_datagram(DatagramIterator &scan);
};
INLINE ostream &operator << (ostream &out, const RNCrowdAgentParams & params);

///ValueList template
template<typename Type>
class ValueList
{
PUBLISHED:
	ValueList(unsigned int size=0);
	ValueList(const ValueList &copy);
	INLINE ~ValueList();

	INLINE void operator =(const ValueList &copy);
	INLINE bool operator== (const ValueList &other) const;
	INLINE void add_value(const Type& value);
	bool remove_value(const Type& value);
	bool has_value(const Type& value) const;
	void add_values_from(const ValueList &other);
	void remove_values_from(const ValueList &other);
	INLINE void clear();
	INLINE int get_num_values() const;
	INLINE Type get_value(int index) const;
	MAKE_SEQ(get_values, get_num_values, get_value);
	INLINE Type operator [](int index) const;
	INLINE int size() const;
	INLINE void operator +=(const ValueList &other);
	INLINE ValueList operator +(const ValueList &other) const;

#ifndef CPPPARSER
public:
	operator plist<Type>() const;
	operator pvector<Type>() const;
#endif //CPPPARSER

private:
#ifndef CPPPARSER
	pvector<Type> _values;
#endif //CPPPARSER
};

///Result values
#define RN_SUCCESS 0
#define RN_ERROR -1

///inline
#include "rnTools.I"

#endif /* RNTOOLS_H_ */
