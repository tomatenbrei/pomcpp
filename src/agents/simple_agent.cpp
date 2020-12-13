#include "bboard.hpp"
#include "agents.hpp"
#include "strategy.hpp"
#include "step_utility.hpp"

using namespace bboard;
using namespace bboard::strategy;

//#define DEBUG_SIMPLEAGENT

namespace agents
{

bool _CheckPos(const State& state, int x, int y)
{
    return !util::IsOutOfBounds(x, y) && IS_WALKABLE(state.items[y][x]);
}

SimpleAgent::SimpleAgent()
{
    std::random_device rd;  // non explicit seed
    rng = std::mt19937_64(rd());
}

SimpleAgent::SimpleAgent(long seed)
{
    rng = std::mt19937_64(seed);
}

void SimpleAgent::reset()
{
    // reset the internal state
    moveQueue.count = 0;
    recentPositions.count = 0;
}

bool _HasRPLoop(SimpleAgent& me)
{
    for(int i = 0; i < me.recentPositions.count / 2; i++)
    {
        if(!(me.recentPositions[i] == me.recentPositions[i + 2]))
        {
            return false;
        }
    }

    return true;
}

Move _SampleFromMoveQueue(SimpleAgent& me, const State* state, bool avoidRecent)
{
    if (me.moveQueue.count == 0)
    {
        return Move::IDLE;
    }
    else
    {
        const AgentInfo& a = state->agents[me.id];
        if (avoidRecent)
        {
            SortDirections(me.moveQueue, me.recentPositions, a.x, a.y);
            return me.moveQueue[me.moveQueue.count - 1 - (me.rng() % std::min(2, me.moveQueue.count))];
        }
        return me.moveQueue[me.rng() % me.moveQueue.count];
    }
}

Move _MoveSafeOneSpace(SimpleAgent& me, const State* state, bool avoidRecent)
{
    const AgentInfo& a = state->agents[me.id];
    SafeDirections(*state, me.moveQueue, a.x, a.y);
    return _SampleFromMoveQueue(me, state, avoidRecent);
}

Move _Decide(SimpleAgent& me, const State* state)
{
    const AgentInfo& a = state->agents[me.id];
    FillRMap(*state, me.r, me.id);

    me.danger = IsInDanger(*state, me.id);

    // first priority: escape danger
    if(me.danger > 0)
    {
#ifdef DEBUG_SIMPLEAGENT
        std::cout << me.id << ": escape danger" << std::endl;
#endif
        MoveTowardsSafePlace(*state, me.r, me.danger, me.moveQueue, me.danger);
        if (me.moveQueue.count > 0)
        {
            return _SampleFromMoveQueue(me, state, true);
        }
        else
        {
            // we found no better place.. just idle to check if things get better
            if(_HasRPLoop(me))
            {
                // if we are stuck idling, try to break out with random movement
                return Move(1 + me.rng() % 4);
            }
            else
            {
                return Move::IDLE;
            }
        }
    }
    // second priority: move to enemies and place bombs
    else if(a.bombCount < a.maxBombCount)
    {
#ifdef DEBUG_SIMPLEAGENT
        std::cout << me.id << ": enemies & bombs" << std::endl;
#endif

        // try to destroy enemies
        if(IsAdjacentEnemy(*state, me.id, 2))
        {
#ifdef DEBUG_SIMPLEAGENT
            std::cout << "> Enemy" << std::endl;
#endif
            return Move::BOMB;
        }

        // destroy wood
        if(IsAdjacentItem(*state, me.id, 1, Item::WOOD))
        {
#ifdef DEBUG_SIMPLEAGENT
            std::cout << "> Wood" << std::endl;
#endif
            return Move::BOMB;
        }

        // move towards enemies

        // if you're stuck in a loop try to break out by randomly selecting
        // a (safe) action
        if(_HasRPLoop(me)) {
#ifdef DEBUG_SIMPLEAGENT
            std::cout << "> RL Loop" << std::endl;
#endif
            return _MoveSafeOneSpace(me, state, false);
        }

        MoveTowardsEnemy(*state, me.r, 7, me.moveQueue);
        if (me.moveQueue.count > 0)
        {
#ifdef DEBUG_SIMPLEAGENT
            std::cout << "> Move towards enemy" << std::endl;
#endif
            return _SampleFromMoveQueue(me, state, false);
        }
    }
    // third priority: select powerups
    else {
#ifdef DEBUG_SIMPLEAGENT
        std::cout << me.id << ": powerups" << std::endl;
#endif

        MoveTowardsPowerup(*state, me.r, 2, me.moveQueue);
        if(me.moveQueue.count > 0)
        {
            return _SampleFromMoveQueue(me, state, false);
        }
    }

    // if that did not work, just move somewhere
    return _MoveSafeOneSpace(me, state, true);
}

Move SimpleAgent::act(const State* state)
{
    const AgentInfo& a = state->agents[id];
    Move m = _Decide(*this, state);
    Position p = util::DesiredPosition(a.x, a.y, m);

    if(recentPositions.RemainingCapacity() == 0)
    {
        recentPositions.PopElem();
    }
    recentPositions.AddElem(p);

    return m;
}

void SimpleAgent::PrintDetailedInfo()
{
    for(int i = 0; i < recentPositions.count; i++)
    {
        std::cout << recentPositions[i] << std::endl;
    }
}

}

