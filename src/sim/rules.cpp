#include "../util/defs.cpp"
#include "../util/sstring.cpp"

namespace physics{

// dir represents the direction an object directly below the attractor will move
// specifically, the unit/s^2 of acceleration to apply to an object (0,-1) from the attractor
// (0,r) is towards
// (0,-r) is away
// (r,0) is an anticlockwise orbit
// (-r,0) is a clockwise orbit
// The distributor is always mass

struct _AttractionRule{
	u32 prop, activator, agg_id;
	vec2 dir;
};
struct AttractionRule{
	u32 prop, activator;
	vec2 dir;
	string name;
	_AttractionRule convert(u32 agg_id){
		return {prop, activator, agg_id, dir};
	}
};

thread_local f32 dt, i_theta, tidal_sample_x, tidal_sample_y;
thread_local int prev_prop_count = 0, prev_qnode_size = 0, prev_node_size = 0;
thread_local int prop_count, qnode_size, node_size, attr_count, attr_block_size;
thread_local u32 *cur_aggregates, *cur_aggregates_end;
thread_local _AttractionRule* cur_a_rules, *cur_a_rules_end;

}