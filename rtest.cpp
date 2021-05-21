#include <iostream>
#include "CArcPCIe.h"
#include "CArcDevice.h"
#include "CExpIFace.h"
#include <exception>

constexpr auto Rows = 1024;
constexpr auto Cols = 1024;
constexpr auto BufferSize = Rows * Cols * sizeof( unsigned );

using namespace arc::device;
using std::cout;
using std::cerr;

class LogCallback : public CExpIFace {
public:
	virtual void ExposeCallback( float fElapsedTime ) {
		cout << "Exposed: " << fElapsedTime << "s\n";
	}

	virtual void ReadCallback( int dPixelCount ) {
		cout << "P.Count: " << dPixelCount << "\n";
	}
};

void expose(CArcDevice *dev, float exp) {
	LogCallback cback;
	try {
		dev->Expose(exp, Rows, Cols, false, &cback);
	}
	catch (std::exception& e) {
		cerr << e.what() << '\n';
	}
}

void readout(CArcDevice *dev, unsigned values = 10) {
	unsigned short* pU16Buf = reinterpret_cast<unsigned short *>(dev->CommonBufferVA());

	for (auto i=0; i < values; i++) {
		cout << "Buffer[" << i << " ]: " << pU16Buf[i] << '\n';
	}
}

int progr() {
	CArcDevice* pArcDev = new CArcPCIe();

	pArcDev->Open(0, BufferSize);
	if (pArcDev->IsOpen()) {
		pArcDev->SetupController(true, // Reset controller
					 true, // Test data link
					 false, // Power-on the camera controller
					 Rows,
					 Cols,
					 "tim.lod");

		cout << pArcDev->ToString() << '\n';
		cout << "This is a " << (pArcDev->IsCCD() ? "CCD" : "IR") << " controller\n";

		cout << "Pre-exposure read out\n";
		readout(pArcDev);
		cout << "Setting synthetic image mode\n";
		pArcDev->SetSyntheticImageMode(true);
		if (pArcDev->IsSyntheticImageMode()) {
			cout << "Synthetic image mode is set\n";
			cout << "Exposing\n";
			expose(pArcDev, 0.5);
			cout << "Post-exposure read out\n";
			readout(pArcDev);
		}

		pArcDev->Close();
		delete pArcDev;

		return 0;
	}

	return -2;
}

int main(int argc, char *argv[]) {
	CArcPCIe::FindDevices();
	if ( CArcPCIe::DeviceCount() > 0 ) {
		return progr();
	}

	return -1;
}
