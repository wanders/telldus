#include "ProtocolSilvanChip.h"
#include "Strings.h"

int ProtocolSilvanChip::methods() const {
	if (TelldusCore::comparei(model(), L"kp100")) {
		return TELLSTICK_UP | TELLSTICK_DOWN | TELLSTICK_STOP | TELLSTICK_LEARN;
	}
	return TELLSTICK_TURNON | TELLSTICK_TURNOFF | TELLSTICK_LEARN;
}

std::string ProtocolSilvanChip::getStringForMethod(int method, unsigned char data, Controller *controller) {
	const unsigned char S = 100;
	const unsigned char L = 255;
	const std::string LONG = "\255\1\200";

	const std::string ONE = LONG + "\100";
	const std::string ZERO = "\100" + LONG;
	std::string strReturn = "R\1S";

	strReturn.append(1, S);
	strReturn.append(1, L);
	strReturn.append(1, 1);
	strReturn.append(1, L);
	strReturn.append(1, 1);
	strReturn.append(1, L);
	strReturn.append(1, 1);
	strReturn.append(1, L);
	strReturn.append(1, 1);
	strReturn.append(1, L);
	strReturn.append(1, 1);
	strReturn.append(1, L);
	strReturn.append(1, 1);
	strReturn.append(1, L);
	strReturn.append(1, 1);
	strReturn.append(1, L);
	strReturn.append(1, 1);
	strReturn.append(1, L);
	strReturn.append(1, 1);
	strReturn.append(1, L);
	strReturn.append(1, 1);
	strReturn.append(1, L);
	strReturn.append(1, 1);
	strReturn.append(1, L);
	strReturn.append(1, 1);
	strReturn.append(1, S);

	int intHouse = this->getIntParameter(L"house", 1, 1048575);

	for( int i = 19; i >= 0; --i ) {
		if (intHouse & (1 << i)) {
			strReturn.append(ONE);
		} else {
			strReturn.append(ZERO);
		}
	}

	if (method == TELLSTICK_TURNOFF || method == TELLSTICK_UP) {
		strReturn.append(ZERO);
		strReturn.append(ZERO);
		strReturn.append(ONE);
		strReturn.append(ZERO);
	} else if (method == TELLSTICK_TURNON || method == TELLSTICK_DOWN) {
		strReturn.append(ONE);
		strReturn.append(ZERO);
		strReturn.append(ZERO);
		strReturn.append(ZERO);
	} else if (method == TELLSTICK_STOP) {
		strReturn.append(ZERO);
		strReturn.append(ONE);
		strReturn.append(ZERO);
		strReturn.append(ZERO);
	} else if (method == TELLSTICK_LEARN) {
		strReturn.append(ZERO);
		strReturn.append(ZERO);
		strReturn.append(ZERO);
		strReturn.append(ONE);
	} else {
		return "";
	}

	strReturn.append(ZERO);
	strReturn.append("+");
	return strReturn;

}
