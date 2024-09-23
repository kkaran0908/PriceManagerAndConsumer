#include <iostream>

enum InstrumentSymbol
{
	AAA = 0,
	DDD = 1,
	WWW = 2,
	ZZZ = 3,
	PPP = 4,
	HHH = 5,
};

std::string SymbolToString(const InstrumentSymbol symbol)
{
	std::string symbolName = "UnknownSymbol";
	switch (symbol)
	{
	case 0:
		symbolName = "AAA";
		break;
	case 1:
		symbolName = "DDD";
		break;
	case 2:
		symbolName = "WWW";
		break;
	case 3:
		symbolName = "ZZZ";
		break;
	case 4:
		symbolName = "PPP";
		break;
	case 5:
		symbolName = "HHH";
		break;
	default:
		symbolName = "UnknownSymbol";
		break;
	}
	return symbolName;
}