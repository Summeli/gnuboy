/* gpSP4Symbian
 *
 * Copyright (C) 2009 Summeli <summeli@summeli.fi>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */


#include <MdaAudioOutputStream.h>
#include <Mda\Common\Audio.h>

#include "antaudio.h"
#include "sound_symbian.h"
#include "cutedebug.h"

#define KNoSegs  3

#define KCallBackInterval 50000
#define KCallBackDelayFromStart 50000

extern "C" void sound_mix(short *sndbuf,int len);

//const int AudioBufferSize =   2 * 2 * 551; //16bit * stereo * samples (11khz) in 50ms

// 2(magic?) * 16bit * stereo * samples (11025hz) in 50ms
const int AudioBufferSize =  2 * 2 * 2 * 550; // 2(magic?) * 16bit * stereo * samples (

CAntAudio::CAntAudio(TInt aRate, TBool aStereo, TInt aVolume )
: iRate(aRate), iStereo(aStereo), iVolume( aVolume )
{

}


CAntAudio* CAntAudio::NewL(TInt aRate, TBool aStereo, TInt aVolume )
{
	CAntAudio* self = new(ELeave) CAntAudio(aRate, aStereo, aVolume );
	CleanupStack::PushL(self);
	self->ConstructL();
	CleanupStack::Pop(self);		// self
	return self;
}


CAntAudio::~CAntAudio()
{
	if(iMdaAudioOutputStream)
		{
		Stop();
		delete iMdaAudioOutputStream;
		}
	
	for (TInt i=0 ; i<KSoundBuffers+1 ; i++)
		delete iSoundBuffers[i];
	
	delete iWait;
	iEmptyBufQ.Reset();
	iEmptyBufQ.Close();
}


void CAntAudio::ConstructL( )
{

	switch(iRate) {
		case 11025: iMdaAudioDataSettings.iSampleRate = TMdaAudioDataSettings::ESampleRate11025Hz; break;
		case 16000: iMdaAudioDataSettings.iSampleRate = TMdaAudioDataSettings::ESampleRate16000Hz; break;
		case 22050: iMdaAudioDataSettings.iSampleRate = TMdaAudioDataSettings::ESampleRate22050Hz; break;
		case 44100: iMdaAudioDataSettings.iSampleRate = TMdaAudioDataSettings::ESampleRate44100Hz; break;
		default:    iMdaAudioDataSettings.iSampleRate = TMdaAudioDataSettings::ESampleRate8000Hz;  break;
	}
			 
	iMdaAudioDataSettings.iChannels   = (iStereo) ? TMdaAudioDataSettings::EChannelsStereo : TMdaAudioDataSettings::EChannelsMono;
	iMdaAudioDataSettings.iCaps       = TMdaAudioDataSettings::ESampleRateFixed | iMdaAudioDataSettings.iSampleRate;
	iMdaAudioDataSettings.iFlags      = TMdaAudioDataSettings::ENoNetworkRouting;
	
	TInt bytesPerFrame = AudioBufferSize;
	
	for (TInt i=0 ; i<KSoundBuffers ; i++)
	{
		//TODO: calculate correct buffer size!
		iSoundBuffers[i] = HBufC8::NewL(bytesPerFrame );
		iSoundBuffers[i]->Des().FillZ  (bytesPerFrame );
		TInt err = iEmptyBufQ.Append(i);
		User::LeaveIfError(err);
	}

	iFrameCount = 0;
	iMdaAudioOutputStream = CMdaAudioOutputStream::NewL(*this);
	
	iWait = new (ELeave) CActiveSchedulerWait;
}

void CAntAudio::Reset()
	{
	iMdaAudioOutputStream->Open(&iMdaAudioDataSettings);
		
    iWait->Start();
	iMdaAudioOutputStream->SetPriority(EPriorityMuchMore, EMdaPriorityPreferenceTime);
	TInt MaxVol = iMdaAudioOutputStream->MaxVolume();
	TInt vol = (MaxVol * iVolume) / MaxVol;
	iMdaAudioOutputStream->SetVolume(vol);
	}

// returns a pointer to buffer for next frame,
// to be used when iSoundBuffers are used directly
TUint8 *CAntAudio::NextFrameL()
{
	if( (iEmptyBufQ.Count() < 2) )
		{
		return NULL;
		}
	return (TUint8*) iSoundBuffers[iEmptyBufQ[0]]->Ptr();
}

void CAntAudio::FrameMixed()
	{
	TInt nextFrame = 0;
	TBool bufferComplete = EFalse;
	
	nextFrame = iEmptyBufQ[0];
	iEmptyBufQ.Remove(0);
	iEmptyBufQ.Compress();
	iMdaAudioOutputStream->WriteL(*iSoundBuffers[nextFrame]);
	}

void CAntAudio::Stop()
	{
	iMdaAudioOutputStream->Stop();
	TInt queuedbuffers = KSoundBuffers - iEmptyBufQ.Count();
	for( TInt i= queuedbuffers; i>0; i--)
		{
		//iWait->Start();
		}
	}

TInt CAntAudio::FreeBufferCount()
	{
	return iEmptyBufQ.Count();
	}

void CAntAudio::SetVolume( TInt aVolume )
	{
	TInt max = iMdaAudioOutputStream->MaxVolume();
	TInt vol = max/10 * aVolume;
	iMdaAudioOutputStream->SetVolume( vol );
	}
void CAntAudio::MaoscOpenComplete(TInt aError)
{
    if (aError==KErrNone)
    {
	//iMdaAudioOutputStream->SetPriority(EPriorityMuchMore, EMdaPriorityPreferenceNone);
    }
    if( aError == KErrServerBusy )
    	{
    	//iMdaAudioOutputStream->Open(&iMdaAudioDataSettings);
    	}
    iWait->AsyncStop();
}

void CAntAudio::MaoscBufferCopied(TInt aError, const TDesC8& aBuffer)
{
	if( aBuffer.Ptr() == iSoundBuffers[0]->Ptr())
		iEmptyBufQ.Append(0);
	else if(  aBuffer.Ptr() == iSoundBuffers[1]->Ptr() )
		iEmptyBufQ.Append(1);
	else if(  aBuffer.Ptr() == iSoundBuffers[2]->Ptr() )
		iEmptyBufQ.Append(2);
	else if(  aBuffer.Ptr() == iSoundBuffers[3]->Ptr() )
		iEmptyBufQ.Append(3);

	//if( !iPlaying )
	//	iWait->AsyncStop();
}
void CAntAudio::MaoscPlayComplete(TInt aError)
{
	//iPlaying = EFalse;
}

static CAntAudio* output = NULL;
static CPeriodic* periodictimer = NULL;


static TInt AudioCallback(TAny* ptr)
	{
	int mixed;
	//get the audiobuffer
	short *coreAudioBuffer = (short*) output->NextFrameL();
	if( coreAudioBuffer )
		{
		//Mix it
		sound_mix( coreAudioBuffer, AudioBufferSize );
		output->FrameMixed();
		}
	else
		{
		__DEBUG1("frame not mixed, not enough free buffers");
		}
	}

void Init_Symbian_Audio( int samplerate )
	{		
	output = CAntAudio::NewL( samplerate, ETrue, 0);
	output->Reset();
	
	
	periodictimer = CPeriodic::NewL( EPriorityNormal ); 
	}

void DeInit_SYmbian_Audio()
	{
	if( periodictimer->IsActive() )
		periodictimer->Cancel();
	delete periodictimer;
	delete output;
	}

void Start_Symbian_Audio( int vol )
	{
	if( !periodictimer->IsActive() )
		{
		short *coreAudioBuffer = (short*) output->NextFrameL();
		output->FrameMixed();
		
		coreAudioBuffer = (short*) output->NextFrameL();
		output->FrameMixed();
		
		output->SetVolume(vol);
		periodictimer->Start( KCallBackDelayFromStart, KCallBackInterval, TCallBack( AudioCallback, NULL ) );
		}
	}

void set_SymbianVolume( int volume )
	{
	output->SetVolume( volume );
	}
void Stop_Symbian_Audio()
	{
	if( periodictimer->IsActive() )
		output->SetVolume(0);
		periodictimer->Cancel();
	}
