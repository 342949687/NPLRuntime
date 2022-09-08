#pragma once
#include <map>
#include <set>

#define USE_CUT_CFG 1 //�Ƿ�ʹ��Ԥ��Ĳü�����������ʵʱ������ࣩ

namespace ParaEngine {
	struct FastCutInfo {

		/*
		local modelName2Id = {
			stairs = 1,
			slab = 2,
			slope = 3,
		}
		*/
		// modelName*10000000 + blockIdx* 100000 + neighborDir*1000 + modelName*100 + blockIdx * 1
		int name;
		int faces[10];
	};

	class BlockTessellateFastCutCfg {
	public:
		static bool isInited;
		static std::map<int, std::set<int>> _fastCutMap;
		static void init();
		static const int getIntFromModelName(std::string & name);
	};
}
