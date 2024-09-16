//===- GraphTraversal.cpp -- Graph algorithms ------------------//
//
//                     SVF: Static Value-Flow Analysis
//
// Copyright (C) <2013-2022>  <Yulei Sui>
//

// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Affero General Public License for more details.

// You should have received a copy of the GNU Affero General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//
//===----------------------------------------------------------------------===//
/*
 * Graph reachability and constraint solving on a self-defined graph
 *
 * Created on: Feb 18, 2024
 */

#include "GraphAlgorithm.h"
#include <sstream>

using namespace std;

/// TODO: Implement your depth-first search here to traverse each program path from src to dst (each node appears at most once in each path).
/// Add each path as a string into std::set<std::string> paths.
/// Each path should have a format like this: "START->1->2->4->5->END", where -> indicates an edge connecting two node IDs.
void Graph::reachability(Node* src, Node* dst) {
	/// TODO: your code starts from here
	this->path.push_back(src->getNodeID());
	this->visited.insert(src);
	if (src == dst)
	{
		std::stringstream ss;
		for (auto &&id : this->path)
		{
			ss << id;
			ss << "->";
		}
		this->paths.insert("START->" + ss.str() + "END");
		this->path.pop_back();
		this->visited.erase(src);
		return;
	}
	for (auto &&edge : src->getOutEdges())
	{
		auto node = edge->getDst();
		if (this->visited.find(node) == this->visited.end()){
			this->reachability(node, dst);
		}
	}
	this->path.pop_back();
	this->visited.erase(src);
}

/// TODO: Implement constraint solving by iteratively (1) propagating points-to sets among nodes on CGraph, and (2)
/// adding new copy edges until a fixed point is reached (i.e., no new copy edges are added). 
/// The solving rules are as follows: 
/// p <--ADDR-- o   =>  pts(p) = pts(p) ∪ {o}
/// q <--COPY-- p   =>  pts(q) = pts(q) ∪ pts(p) 
/// q <--LOAD-- p   =>  for each o ∈ pts(p) : q <--COPY-- o 
/// q <--STORE-- p  =>  for each o ∈ pts(q) : o <--COPY-- p 
/// pts(q) denotes the points-to set of node q. 
/// Refer to the APIs in CGraph, including `addPts`, `getPts`, `unionPts` and `addEdge` for your implementation.
void CGraph::solveWorklist() {
	/// TODO: your code starts from here
	for (auto &&edge : this->edges)
	{
		this->pushIntoWorklist(edge->getSrc()->getID());
	}
	
	while (true)
	{
		if (this->worklist.empty())
			break;
		auto nodeId = this->popFromWorklist();
		auto node = this->getNode(nodeId);
		auto outEdges = node->getOutEdges();
		for (auto &&edge : outEdges)
		{
			auto type = edge->getType();
			auto p = edge->getDst();
			switch (type)
			{
			case CGEdge::ADDR:
				// p <--ADDR-- o   =>  pts(p) = pts(p) ∪ {o}
				this->addPts(p, node);
				break;
			case CGEdge::COPY:
				// q <--COPY-- p   =>  pts(q) = pts(q) ∪ pts(p) 
				this->unionPts(p, node);
				break;
			case CGEdge::LOAD:
				// q <--LOAD-- p   =>  for each o ∈ pts(p) : q <--COPY-- o 
				for (auto &&id : node->getPts()){
					if (this->addEdge(this->getNode(id), p, CGEdge::COPY)){
						this->pushIntoWorklist(id);
						this->pushIntoWorklist(id);
					}
				}
				break;
			case CGEdge::STORE:
				// q <--STORE-- p  =>  for each o ∈ pts(q) : o <--COPY-- p 
				for (auto &&id : p->getPts()){
					// add new COPY edge to graph, push it into worklist if not exist
					if (this->addEdge(node, this->getNode(id), CGEdge::COPY)){
						this->pushIntoWorklist(node->getID());
						this->pushIntoWorklist(id);
					}
				}
				break;
			default:
				break;
			}
		}
	}
}