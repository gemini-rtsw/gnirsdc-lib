#include <iostream>
#include "CArcDevice.h"
#include "CArcPCIe.h"
#include "ArcDefs.h"
#include "CArcFitsFile.h"

using namespace std;
using namespace arc;
using namespace arc::device;

using namespace arc::fits;

// Expose Callback interface
// class CExposeListener : public CExpIFace
// {
  // void ExposeCallback(float elapsedTime)
  // {
    // cout << "Expose Callback - ellapsed time: " << elapsedTime << endl;
  // }

  // void ReadCallback(int pixelCount)
  // {
    // cout << "Read Callback - pixel count: " << pixelCount << endl;
  // }
// };

int main(int argc, char **argv)
{

  // CExposeListener cExposeListener;

  CArcPCIe::FindDevices();

  if(CArcPCIe::DeviceCount() > 0)
  {
    cout << "Found " << CArcPCIe::DeviceCount() << " device(s)" << endl;

    CArcDevice *pArcDev = new CArcPCIe();

    pArcDev->Open(0);
    if(pArcDev->IsOpen())
    {
      cout << "Device opened sucessfully" << endl;
    }
    else
    {
      cerr << "Device failed to open!. This almost certainly won't work" << endl;
    }

    cout << "Device Found: " << pArcDev->ToString() << endl;
    cout << "Device Status: " << pArcDev->GetStatus() << endl;

    if(pArcDev->IsControllerConnected())
    {
      cout << "Yay! Controller is connected." << endl;
    }
    else
    {
      cout << "Uh-oh! Controller is NOT connected." << endl;
      cout << "Check power. Check fiber." << endl;
    }

    // Reset Controller in case it was left in a bad state
    cout << "Resetting Controller..." << flush;
    pArcDev->ResetController();
    cout << " OK." << endl;

    // Return value variable
    int dRetVal;

    // Load the timing board DSP code
    cout << "Uploading Timing Board Run-time DSP code... " << flush;
    pArcDev->LoadControllerFile("tim.lod");
    cout << "done" << endl;
    cout << "Device Status: " << pArcDev->GetStatus() << endl;

    // Send  a bunch of Test Data Link Commands
    cout << "Multiple TDL test starting..." << flush;
    good_tdl=0;
    bad_tdl=0;
    for(int i=0; i<50000; i++)
    {
      int r = rand() % 65535;
      int dRetVal = pArcDev->Command(TIM_ID, TDL, r);
      if(dRetVal == r)
      {
        good_tdl++;
      }
      else
      {
        bad_tdl++;
        cout << "...  TDL " << i << " Failed! - got: " << dRetVal << endl;
      }
    }
    cout << "... Got " << good_tdl << " good TDL results and " << bad_tdl << " bad TDL results" << endl;


    // Try and do my VER command
    cout << "Attempting VER command... " << flush;
    dRetVal = pArcDev->Command(TIM_ID, 0x00564552);
    cout << "got software Version: " << dRetVal << endl;
    
    cout << "Setting 512x512 image size... " << flush;
    pArcDev->SetImageSize(512, 512);
    cout << " OK" << endl;

    cout << "Mapping Common Buffer... " << flush;
    int bufSize = 512 * 512 * 2;
    pArcDev->MapCommonBuffer(bufSize);
    if(pArcDev->CommonBufferSize() == bufSize)
    {
      cout << "Common Buffer Mapped OK" << endl;
    }
    else
    {
      cout << "Failed to Map Common Buffer" << endl;
    }

    cout << "Setting Synthetic Image Mode... " << flush;
    pArcDev->SetSyntheticImageMode(true);
    if(pArcDev->IsSyntheticImageMode())
    {
      cout << "Synthetic Image Mode set" << endl;
    }
    else
    {
      cout << "Synthetic Image Mode NOT set" << endl;
    }
 
    cout << "Quote-Unquote Exposing... " << flush;
    pArcDev->Expose(0.0, 512, 512, NULL, NULL, false);
    cout << "Seemed to expose OK" << endl;

    // See what's in the buffer
    unsigned short *buf = (unsigned short *)pArcDev->CommonBufferVA();
    for (int i=0; i<10; i++)
    {
      cout << "Buffer[" << i << "]= " << buf[i] << endl;
    }

    // Write FITS file
    cout << "Writing FITS file... " << flush;
    CArcFitsFile cFits("test.fits", 512, 512);
    cFits.Write(buf);
    cout << "OK." << endl;


    pArcDev->Close();
  }
  else
  {
    cout << "No Devices Found" << endl;
  }

  return 0;
}
