//(c) 2016 by Authors
//This file is a part of ABruijn program.
//Released under the BSD license (see LICENSE file)

#include <vector>
#include <iostream>
#include <unordered_set>
#include <unordered_map>

#include "../common/config.h"
#include "../common/logger.h"
#include "chimera.h"



bool ChimeraDetector::isChimeric(FastaRecord::Id readId)
{
	if (!_chimeras.count(readId))
	{
		bool result = this->testReadByCoverage(readId);
		_chimeras[readId] = result;
		_chimeras[readId.rc()] = result;
	}
	return _chimeras[readId];
}


bool ChimeraDetector::testReadByCoverage(FastaRecord::Id readId)
{
	static const int WINDOW = Constants::chimeraWindow;
	const int FLANK = Constants::maximumOverhang / WINDOW;

	std::vector<int> coverage;
	int numWindows = _seqContainer.seqLen(readId) / WINDOW;
	if (numWindows - 2 * FLANK <= 0) return true;

	coverage.assign(numWindows - 2 * FLANK, 0);
	for (auto& ovlp : _ovlpContainer.lazySeqOverlaps(readId))
	{
		if (ovlp.curId == ovlp.extId.rc()) continue;

		for (int pos = ovlp.curBegin / WINDOW + 1; 
			 pos < ovlp.curEnd / WINDOW; ++pos)
		{
			if (pos - FLANK >= 0 && 
				pos - FLANK < (int)coverage.size())
			{
				++coverage[pos - FLANK];
			}
		}
	}

	std::string covStr;
	for (int cov : coverage)
	{
		covStr += std::to_string(cov) + " ";
	}

	int maxCoverage = *std::max_element(coverage.begin(), coverage.end());
	for (auto cov : coverage)
	{
		if (cov == 0) return true;

		if ((float)maxCoverage / cov > (float)Constants::maxCoverageDropRate) 
		{
			//Logger::get().debug() << "Chimeric: " 
			//	<< _seqContainer.seqName(readId) << covStr;
			return true;
		}
	}

	//self overlaps
	for (auto& ovlp : _ovlpContainer.lazySeqOverlaps(readId))
	{
		if (ovlp.curId == ovlp.extId.rc()) 
		{
			Logger::get().debug() << "Self-ovlp: " 
				<< _seqContainer.seqName(readId) << covStr;
			return true;
		}
	}

	return false;
}