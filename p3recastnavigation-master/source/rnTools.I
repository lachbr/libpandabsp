/**
 * \file rnTools.I
 *
 * \date 2016-03-19
 * \author consultit
 */

#ifndef RNTOOLS_I_
#define RNTOOLS_I_


///ValueList
/**
 *
 */
template<typename Type>
INLINE ValueList<Type>::ValueList(unsigned int size):_values(size)
{
}

/**
 *
 */
template<typename Type>
INLINE ValueList<Type>::ValueList(const ValueList &copy) :
		_values(copy._values)
{
}

/**
 *
 */
template<typename Type>
INLINE ValueList<Type>::~ValueList()
{
}

/**
 * Adds a new value to the collection.
 */
template<typename Type>
INLINE void ValueList<Type>::add_value(const Type& value)
{
	_values.push_back(value);
}

/**
 *
 */
template<typename Type>
INLINE void ValueList<Type>::operator =(const ValueList &copy)
{
	_values = copy._values;
}

/**
 *
 */
template<typename Type>
INLINE bool ValueList<Type>::operator== (const ValueList &other) const
{
	return _values == other._values;
}

/**
 * Removes all values from the collection.
 */
template<typename Type>
INLINE void ValueList<Type>::clear()
{
	_values.clear();
}

/**
 * Returns the number of values in the collection.
 */
template<typename Type>
INLINE int ValueList<Type>::get_num_values() const
{
	return _values.size();
}

/**
 * Returns the nth value in the collection.
 */
template<typename Type>
INLINE Type ValueList<Type>::get_value(int index) const
{
	nassertr_always((index >= 0) && (index < (int ) _values.size()), Type())

	return _values[index];
}

/**
 * Returns the nth value in the collection.  This is the same as
 * get_value(), but it may be a more convenient way to access it.
 */
template<typename Type>
INLINE Type ValueList<Type>::operator [](int index) const
{
	nassertr_always((index >= 0) && (index < (int ) _values.size()), Type())

	return _values[index];
}

/**
 * Returns the number of values in the collection.  This is the same thing as
 * get_num_values().
 */
template<typename Type>
INLINE int ValueList<Type>::size() const
{
	return _values.size();
}

/**
 * Appends the other list onto the end of this one.
 */
template<typename Type>
INLINE void ValueList<Type>::operator +=(const ValueList &other)
{
	add_values_from(other);
}

/**
 * Returns a ValueList representing the concatenation of the two
 * lists.
 */
template<typename Type>
INLINE ValueList<Type> ValueList<Type>::operator +(const ValueList &other) const
{
	ValueList a(*this);
	a += other;
	return a;
}

/**
 * Removes the indicated value from the collection.  Returns true if
 * the value was removed, false if it was not a member of the collection.
 */
template<typename Type>
bool ValueList<Type>::remove_value(const Type& value)
{
	typename pvector<Type>::iterator iter = find(_values.begin(),
			_values.end(), value);
	CONTINUE_IF_ELSE_R(iter != _values.end(), false)

	_values.erase(iter);
	return true;
}

/**
 * Adds all the values indicated in the other collection to this value.
 * The other values are simply appended to the end of the values in this list;
 * duplicates are not automatically removed.
 */
template<typename Type>
void ValueList<Type>::add_values_from(const ValueList &other)
{
	int other_num_values = other.get_num_values();
	for (int i = 0; i < other_num_values; i++)
	{
		add_value(other.get_value(i));
	}
}

/**
 * Removes from this collection all of the values listed in the other
 * collection.
 */
template<typename Type>
void ValueList<Type>::remove_values_from(const ValueList& other)
{
	pvector<Type> new_values;
	int num_values = get_num_values();
	for (int i = 0; i < num_values; i++)
	{
		const Type& value = get_value(i);
		if (!other.has_value(value))
		{
			new_values.push_back(value);
		}
	}
	_values = new_values;
}

/**
 * Returns true if the indicated value appears in this collection,
 * false otherwise.
 */
template<typename Type>
bool ValueList<Type>::has_value(const Type& value) const
{
	typename pvector<Type>::const_iterator iter = find(_values.begin(),
			_values.end(), value);

	return iter != _values.end() ? true : false;
}

#ifndef CPPPARSER
template<typename Type>
inline ValueList<Type>::operator plist<Type>() const
{
	plist<Type> valueList;
	for (typename pvector<Type>::size_type i = 0; i < _values.size(); ++i)
	{
		valueList.push_back(_values[i]);
	}
	return valueList;
}
template<typename Type>
inline ValueList<Type>::operator pvector<Type>() const
{
	return _values;
}
#endif //CPPPARSER

///NavMeshSettings
INLINE float RNNavMeshSettings::get_cellSize() const
{
	return _navMeshSettings.m_cellSize;
}
INLINE void RNNavMeshSettings::set_cellSize(float value)
{
	_navMeshSettings.m_cellSize = value;
}
INLINE float RNNavMeshSettings::get_cellHeight() const
{
	return _navMeshSettings.m_cellHeight;
}
INLINE void RNNavMeshSettings::set_cellHeight(float value)
{
	_navMeshSettings.m_cellHeight = value;
}
INLINE float RNNavMeshSettings::get_agentHeight() const
{
	return _navMeshSettings.m_agentHeight;
}
INLINE void RNNavMeshSettings::set_agentHeight(float value)
{
	_navMeshSettings.m_agentHeight = value;
}
INLINE float RNNavMeshSettings::get_agentRadius() const
{
	return _navMeshSettings.m_agentRadius;
}
INLINE void RNNavMeshSettings::set_agentRadius(float value)
{
	_navMeshSettings.m_agentRadius = value;
}
INLINE float RNNavMeshSettings::get_agentMaxClimb() const
{
	return _navMeshSettings.m_agentMaxClimb;
}
INLINE void RNNavMeshSettings::set_agentMaxClimb(float value)
{
	_navMeshSettings.m_agentMaxClimb = value;
}
INLINE float RNNavMeshSettings::get_agentMaxSlope() const
{
	return _navMeshSettings.m_agentMaxSlope;
}
INLINE void RNNavMeshSettings::set_agentMaxSlope(float value)
{
	_navMeshSettings.m_agentMaxSlope = value;
}
INLINE float RNNavMeshSettings::get_regionMinSize() const
{
	return _navMeshSettings.m_regionMinSize;
}
INLINE void RNNavMeshSettings::set_regionMinSize(float value)
{
	_navMeshSettings.m_regionMinSize = value;
}
INLINE float RNNavMeshSettings::get_regionMergeSize() const
{
	return _navMeshSettings.m_regionMergeSize;
}
INLINE void RNNavMeshSettings::set_regionMergeSize(float value)
{
	_navMeshSettings.m_regionMergeSize = value;
}
INLINE float RNNavMeshSettings::get_edgeMaxLen() const
{
	return _navMeshSettings.m_edgeMaxLen;
}
INLINE void RNNavMeshSettings::set_edgeMaxLen(float value)
{
	_navMeshSettings.m_edgeMaxLen = value;
}
INLINE float RNNavMeshSettings::get_edgeMaxError() const
{
	return _navMeshSettings.m_edgeMaxError;
}
INLINE void RNNavMeshSettings::set_edgeMaxError(float value)
{
	_navMeshSettings.m_edgeMaxError = value;
}
INLINE float RNNavMeshSettings::get_vertsPerPoly() const
{
	return _navMeshSettings.m_vertsPerPoly;
}
INLINE void RNNavMeshSettings::set_vertsPerPoly(float value)
{
	_navMeshSettings.m_vertsPerPoly = value;
}
INLINE float RNNavMeshSettings::get_detailSampleDist() const
{
	return _navMeshSettings.m_detailSampleDist;
}
INLINE void RNNavMeshSettings::set_detailSampleDist(float value)
{
	_navMeshSettings.m_detailSampleDist = value;
}
INLINE float RNNavMeshSettings::get_detailSampleMaxError() const
{
	return _navMeshSettings.m_detailSampleMaxError;
}
INLINE void RNNavMeshSettings::set_detailSampleMaxError(float value)
{
	_navMeshSettings.m_detailSampleMaxError = value;
}
INLINE int RNNavMeshSettings::get_partitionType() const
{
	return _navMeshSettings.m_partitionType;
}
INLINE void RNNavMeshSettings::set_partitionType(int value)
{
	_navMeshSettings.m_partitionType = value;
}
INLINE ostream &operator << (ostream &out, const RNNavMeshSettings & settings)
{
	settings.output(out);
	return out;
}

///NavMeshTileSettings
INLINE bool RNNavMeshTileSettings::get_buildAllTiles() const
{
	return _navMeshTileSettings.m_buildAllTiles;
}
INLINE void RNNavMeshTileSettings::set_buildAllTiles(bool value)
{
	_navMeshTileSettings.m_buildAllTiles = value;
}
INLINE int RNNavMeshTileSettings::get_maxTiles() const
{
	return _navMeshTileSettings.m_maxTiles;
}
INLINE void RNNavMeshTileSettings::set_maxTiles(int value)
{
	_navMeshTileSettings.m_maxTiles = value;
}
INLINE int RNNavMeshTileSettings::get_maxPolysPerTile() const
{
	return _navMeshTileSettings.m_maxPolysPerTile;
}
INLINE void RNNavMeshTileSettings::set_maxPolysPerTile(int value)
{
	_navMeshTileSettings.m_maxPolysPerTile = value;
}
INLINE float RNNavMeshTileSettings::get_tileSize() const
{
	return _navMeshTileSettings.m_tileSize;
}
INLINE void RNNavMeshTileSettings::set_tileSize(float value)
{
	_navMeshTileSettings.m_tileSize = value;
}
INLINE ostream &operator << (ostream &out, const RNNavMeshTileSettings & settings)
{
	settings.output(out);
	return out;
}

///Convex volume settings.
INLINE bool RNConvexVolumeSettings::operator==(
		const RNConvexVolumeSettings &other) const
{
	return (_area == other._area) && (_flags == other._flags)
			&& (_ref == other._ref);
}
INLINE int RNConvexVolumeSettings::get_area() const
{
	return _area;
}
INLINE void RNConvexVolumeSettings::set_area(int value)
{
	_area = value;
}
INLINE int RNConvexVolumeSettings::get_flags() const
{
	return _flags;
}
INLINE void RNConvexVolumeSettings::set_flags(int value)
{
	_flags = value;
}
INLINE LPoint3f RNConvexVolumeSettings::get_centroid() const
{
	return _centroid;
}
INLINE void RNConvexVolumeSettings::set_centroid(LPoint3f value)
{
	_centroid = value;
}
INLINE int RNConvexVolumeSettings::get_ref() const
{
	return _ref;
}
INLINE void RNConvexVolumeSettings::set_ref(int value)
{
	_ref = value;
}
INLINE ostream &operator << (ostream &out, const RNConvexVolumeSettings & settings)
{
	settings.output(out);
	return out;
}

///Off mesh connection settings.
INLINE bool RNOffMeshConnectionSettings::operator==(
		const RNOffMeshConnectionSettings &other) const
{
	return (_rad == other._rad) && (_bidir == other._bidir)
			&& (_area == other._area) && (_flags == other._flags)
			&& (_ref == other._ref);
}
INLINE float RNOffMeshConnectionSettings::get_rad() const
{
	return _rad;
}
INLINE void RNOffMeshConnectionSettings::set_rad(float value)
{
	_rad = value;
}
INLINE bool RNOffMeshConnectionSettings::get_bidir() const
{
	return _bidir;
}
INLINE void RNOffMeshConnectionSettings::set_bidir(bool value)
{
	_bidir = value;
}
INLINE unsigned int RNOffMeshConnectionSettings::get_userId() const
{
	return _userId;
}
INLINE void RNOffMeshConnectionSettings::set_userId(unsigned int value)
{
	_userId = value;
}
INLINE int RNOffMeshConnectionSettings::get_area() const
{
	return _area;
}
INLINE void RNOffMeshConnectionSettings::set_area(int value)
{
	_area = value;
}
INLINE int RNOffMeshConnectionSettings::get_flags() const
{
	return _flags;
}
INLINE void RNOffMeshConnectionSettings::set_flags(int value)
{
	_flags = value;
}
INLINE int RNOffMeshConnectionSettings::get_ref() const
{
	return _ref;
}
INLINE void RNOffMeshConnectionSettings::set_ref(int value)
{
	_ref = value;
}
INLINE ostream &operator << (ostream &out, const RNOffMeshConnectionSettings & settings)
{
	settings.output(out);
	return out;
}

///Obstacle settings.
INLINE bool RNObstacleSettings::operator==(
		const RNObstacleSettings &other) const
{
	return (_radius == other._radius) && (_dims == other._dims)
			&& (_ref == other._ref);
}
INLINE float RNObstacleSettings::get_radius() const
{
	return _radius;
}
INLINE void RNObstacleSettings::set_radius(float value)
{
	_radius = value;
}
INLINE LVecBase3f RNObstacleSettings::get_dims() const
{
	return _dims;
}
INLINE void RNObstacleSettings::set_dims(const LVecBase3f& value)
{
	_dims = value;
}
INLINE unsigned int RNObstacleSettings::get_ref() const
{
	return _ref;
}
INLINE void RNObstacleSettings::set_ref(unsigned int value)
{
	_ref = value;
}
INLINE ostream &operator << (ostream &out, const RNObstacleSettings & settings)
{
	settings.output(out);
	return out;
}

///CrowdAgentParams
INLINE float RNCrowdAgentParams::get_radius() const
{
	return _dtCrowdAgentParams.radius;
}
INLINE void RNCrowdAgentParams::set_radius(float value)
{
	_dtCrowdAgentParams.radius = value;
}
INLINE float RNCrowdAgentParams::get_height() const
{
	return _dtCrowdAgentParams.height;
}
INLINE void RNCrowdAgentParams::set_height(float value)
{
	_dtCrowdAgentParams.height = value;
}
INLINE float RNCrowdAgentParams::get_maxAcceleration() const
{
	return _dtCrowdAgentParams.maxAcceleration;
}
INLINE void RNCrowdAgentParams::set_maxAcceleration(float value)
{
	_dtCrowdAgentParams.maxAcceleration = value;
}
INLINE float RNCrowdAgentParams::get_maxSpeed() const
{
	return _dtCrowdAgentParams.maxSpeed;
}
INLINE void RNCrowdAgentParams::set_maxSpeed(float value)
{
	_dtCrowdAgentParams.maxSpeed = value;
}
INLINE float RNCrowdAgentParams::get_collisionQueryRange() const
{
	return _dtCrowdAgentParams.collisionQueryRange;
}
INLINE void RNCrowdAgentParams::set_collisionQueryRange(float value)
{
	_dtCrowdAgentParams.collisionQueryRange = value;
}
INLINE float RNCrowdAgentParams::get_pathOptimizationRange() const
{
	return _dtCrowdAgentParams.pathOptimizationRange;
}
INLINE void RNCrowdAgentParams::set_pathOptimizationRange(float value)
{
	_dtCrowdAgentParams.pathOptimizationRange = value;
}
INLINE float RNCrowdAgentParams::get_separationWeight() const
{
	return _dtCrowdAgentParams.separationWeight;
}
INLINE void RNCrowdAgentParams::set_separationWeight(float value)
{
	_dtCrowdAgentParams.separationWeight = value;
}
INLINE unsigned char RNCrowdAgentParams::get_updateFlags() const
{
	return _dtCrowdAgentParams.updateFlags;
}
INLINE void RNCrowdAgentParams::set_updateFlags(unsigned char value)
{
	_dtCrowdAgentParams.updateFlags = value;
}
INLINE unsigned char RNCrowdAgentParams::get_obstacleAvoidanceType() const
{
	return _dtCrowdAgentParams.obstacleAvoidanceType;
}
INLINE void RNCrowdAgentParams::set_obstacleAvoidanceType(unsigned char value)
{
	_dtCrowdAgentParams.obstacleAvoidanceType = value;
}
INLINE unsigned char RNCrowdAgentParams::get_queryFilterType() const
{
	return _dtCrowdAgentParams.queryFilterType;
}
INLINE void RNCrowdAgentParams::set_queryFilterType(unsigned char value)
{
	_dtCrowdAgentParams.queryFilterType = value;
}
INLINE void* RNCrowdAgentParams::get_userData() const
{
	return _dtCrowdAgentParams.userData;
}
INLINE void RNCrowdAgentParams::set_userData(void* value)
{
	_dtCrowdAgentParams.userData = value;
}
INLINE ostream &operator << (ostream &out, const RNCrowdAgentParams & params)
{
	params.output(out);
	return out;
}

#endif /* RNTOOLS_I_ */
