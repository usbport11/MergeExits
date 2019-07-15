#include "Dungeon.h"

MDungeon::MDungeon():MLevel() {
	MinLeafSize = 0;
	MaxLeafSize = 0;
	MinRoomSize = 0;
}

MDungeon::MDungeon(int TilesCountX, int TilesCountY, int inMinLeafSize, int inMaxLeafSize, int inMinRoomSize):MLevel(TilesCountX, TilesCountY) {
	MinLeafSize = 0;
	MaxLeafSize = 0;
	MinRoomSize = 0;
	if(inMinLeafSize > 0) MinLeafSize = inMinLeafSize;
	if(inMaxLeafSize > 0) MaxLeafSize = inMaxLeafSize;
	if(inMinRoomSize > 0) MinRoomSize = inMinRoomSize;
}

bool MDungeon::Generate() {
	if(!Map) {
		if(!AllocateMap()) return false;
	}
	else Clear();
	
	if(MinRoomSize < 3) {
		return false;
	}
	if(MinRoomSize >= MinLeafSize || MinLeafSize >= MaxLeafSize) {
		return false;
	}
	
	//create tree
	if(!SplitTree(&Tree, TilesCount[0], TilesCount[1], MinLeafSize, MaxLeafSize)) {
		return false;
	}
	
	if(!CreateRooms()) return false;
	if(!Triangulate()) return false;
	if(!CreateHalls()) return false;
	
	MST.clear();
	NodesCenters.clear();	
	ClearTree(&Tree);
	
	if(!ConvertToTiles()) return false;
	
	return true;
}

bool MDungeon::Clear() {
	MLevel::Clear();
}

bool MDungeon::Close() {
	MLevel::Close();
}

bool MDungeon::CreateRooms() {
	//create rooms and fill centers map
	TNode<NRectangle2>* pRoomNode;
	glm::vec2 Center;
	NRectangle2* pRoom;
	stLeaf* pLeaf;
	std::list<TNode<stLeaf>* >::iterator itl;
	int RoomsNumber = 0;
	for(itl = Tree.begin(); itl != Tree.end(); itl++) {
		pRoomNode = CreateRoomInLeaf(*itl, MinRoomSize);
		if(!pRoomNode) continue;
		pRoom = pRoomNode->GetValueP();
		if(!pRoom) continue;
		//add in map
		Center.x = (pRoom->Position.x + pRoom->Size.x * 0.5);// * TileSize.x;
		Center.y = (pRoom->Position.y + pRoom->Size.y * 0.5);// * TileSize.y;
		NodesCenters.insert(std::pair<glm::vec2, TNode<NRectangle2>* >(Center, pRoomNode));
		//add room to level map grid
		FillMap(pRoom->Position.x, pRoom->Position.y, pRoom->Size.x, pRoom->Size.y, 1);
		RoomsNumber ++;
	}
	if(RoomsNumber < 2) {
		return false;
	}
	
	return true;
}

bool MDungeon::Triangulate() {
	glm::vec2 StartPoint;
	glm::vec2 EndPoint;
	float Distance, MaxDistance = 0;
	
	EndPoint = StartPoint = NodesCenters.begin()->first;
	
	//if rooms number less than 2 next steps useless
	if(NodesCenters.size() <= 2) return false;
	
	//copy centers for triangulation
	std::map<glm::vec2, TNode<NRectangle2>*, stVec2Compare>::iterator itm;
	std::vector<glm::vec2> CentersPoints;
	for(itm = NodesCenters.begin(); itm != NodesCenters.end(); itm++) {
		CentersPoints.push_back(itm->first);
		Distance = glm::distance(itm->first, StartPoint);
		if(Distance > MaxDistance) {
			MaxDistance = Distance;
			EndPoint = itm->first;
		}
	}
	
	if(rand() % 2) {
		glm::vec2 Save = StartPoint;
		StartPoint = EndPoint;
		EndPoint = Save;
	}
	nStartPoint[0] = StartPoint.x;
	nStartPoint[1] = StartPoint.y;
	nEndPoint[0] = EndPoint.x;
	nEndPoint[1] = EndPoint.y;
	
	//triangulate by delaunay and get mst
	MDelaunay Triangulation;
	std::vector<MTriangle> Triangles = Triangulation.Triangulate(CentersPoints);
	std::vector<MEdge> Edges = Triangulation.GetEdges();
	MST = Triangulation.CreateMSTEdges();
	
	Triangulation.Clear();
	Triangles.clear();
	Edges.clear();
	CentersPoints.clear();
	
	if(MST.size() <= 0) return false;
	
	return true;
}

bool MDungeon::CreateHalls() {
	std::vector<NRectangle2> Halls;
	TNode<NRectangle2>* pNode0;
	TNode<NRectangle2>* pNode1;
	for(int i=0; i<MST.size(); i++) {
		pNode0 = NodesCenters[MST[i].p1];
		pNode1 = NodesCenters[MST[i].p2];
		Halls = CreateHalls3(pNode0->GetValueP(), pNode1->GetValueP());
		if(Halls.size() <= 0) return false;
		//add hall to level map grid
		for(int j=0; j<Halls.size(); j++) {
			FillMap(Halls[j].Position.x, Halls[j].Position.y, Halls[j].Size.x, Halls[j].Size.y, 1);
		}
		Halls.clear();
	}
	
	return true;
}

int MDungeon::GetType() {
	return LGT_DUNGEON;
}
