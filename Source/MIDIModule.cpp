/*
  ==============================================================================

    MIDIModule.cpp
    Created: 20 Dec 2016 12:35:26pm
    Author:  Ben

  ==============================================================================
*/

#include "MIDIModule.h"
#include "MIDICommands.h"

MIDIModule::MIDIModule(const String & name, bool _useGenericControls) :
	Module(name),
	inputDevice(nullptr),
	outputDevice(nullptr),
	useGenericControls(_useGenericControls)
{
	setupIOConfiguration(true, true);

	//canHandleRouteValues = true;

	midiParam = new MIDIDeviceParameter("Devices");
	moduleParams.addParameter(midiParam);

	if (useGenericControls)
	{
		autoAdd = moduleParams.addBoolParameter("Auto Add", "Auto Add MIDI values that are received but not in the list", false);
		defManager.add(CommandDefinition::createDef(this, "", "Note On", &MIDINoteAndCCCommand::create)->addParam("type", (int)MIDINoteAndCCCommand::NOTE_ON));
		defManager.add(CommandDefinition::createDef(this, "", "Note Off", &MIDINoteAndCCCommand::create)->addParam("type", (int)MIDINoteAndCCCommand::NOTE_OFF));
		defManager.add(CommandDefinition::createDef(this, "", "Full Note", &MIDINoteAndCCCommand::create)->addParam("type", (int)MIDINoteAndCCCommand::FULL_NOTE));
		defManager.add(CommandDefinition::createDef(this, "", "Controller Change", &MIDINoteAndCCCommand::create)->addParam("type", (int)MIDINoteAndCCCommand::CONTROLCHANGE));
		defManager.add(CommandDefinition::createDef(this, "", "Sysex Message", &MIDISysExCommand::create));
	}
	
	setupIOConfiguration(inputDevice != nullptr, outputDevice != nullptr);
}

MIDIModule::~MIDIModule()
{
	if(inputDevice != nullptr) inputDevice->removeMIDIInputListener(this);
	if (outputDevice != nullptr) outputDevice->close();
}


void MIDIModule::sendNoteOn(int pitch, int velocity, int channel)
{
	if (!enabled->boolValue()) return;
	if (outputDevice == nullptr) return;
	if (logOutgoingData->boolValue()) NLOG(niceName, "Send Note on " << MIDIManager::getNoteName(pitch) << ", " << velocity << ", " << channel);
	outActivityTrigger->trigger();
	outputDevice->sendNoteOn(pitch, velocity, channel);
}

void MIDIModule::sendNoteOff(int pitch, int channel)
{
	if (!enabled->boolValue()) return;
	if (outputDevice == nullptr) return;
	if (logOutgoingData->boolValue()) NLOG(niceName, "Send Note off " << MIDIManager::getNoteName(pitch) << ", " << channel);
	outActivityTrigger->trigger();
	outputDevice->sendNoteOff(pitch, channel);
}

void MIDIModule::sendControlChange(int number, int value, int channel)
{
	if (!enabled->boolValue()) return;
	if (outputDevice == nullptr) return;
	if (logOutgoingData->boolValue()) NLOG(niceName, "Send Control Change " << number << ", " << value << ", " << channel);
	outActivityTrigger->trigger();
	outputDevice->sendControlChange(number,value,channel);
}

void MIDIModule::sendSysex(Array<uint8> data)
{
	if (!enabled->boolValue()) return;
	if (outputDevice == nullptr) return;
	if (logOutgoingData->boolValue()) NLOG(niceName, "Send Sysex " << data.size() << " bytes");
	outActivityTrigger->trigger();
	outputDevice->sendSysEx(data);
}


void MIDIModule::onControllableFeedbackUpdateInternal(ControllableContainer * cc, Controllable * c)
{
	Module::onControllableFeedbackUpdateInternal(cc, c);

	if (c == midiParam)
	{
		updateMIDIDevices();
	}
}

void MIDIModule::updateMIDIDevices()
{
	MIDIInputDevice * newInput = midiParam->inputDevice;
	if (inputDevice != newInput)
	{
		if (inputDevice != nullptr)
		{
			inputDevice->removeMIDIInputListener(this);
		}
		inputDevice = newInput;
		if (inputDevice != nullptr)
		{
			inputDevice->addMIDIInputListener(this);
		}
	}

	MIDIOutputDevice * newOutput = midiParam->outputDevice;
	if (outputDevice != newOutput)
	{
		if(outputDevice != nullptr) outputDevice->close();
		outputDevice = newOutput;
		if(outputDevice != nullptr) outputDevice->open();
	} 

	setupIOConfiguration(inputDevice != nullptr || valuesCC.controllables.size() > 0, outputDevice != nullptr);
}

void MIDIModule::noteOnReceived(const int & channel, const int & pitch, const int & velocity)
{
	if (!enabled->boolValue()) return; 
	inActivityTrigger->trigger();
	if (logIncomingData->boolValue())  NLOG(niceName, "Note On : " << channel << ", " << MIDIManager::getNoteName(pitch) << ", " << velocity);

	if (useGenericControls) updateValue(channel, MIDIManager::getNoteName(pitch), velocity);
}

void MIDIModule::noteOffReceived(const int & channel, const int & pitch, const int & velocity)
{
	if (!enabled->boolValue()) return; 
	inActivityTrigger->trigger();
	if (logIncomingData->boolValue()) NLOG(niceName, "Note Off : " << channel << ", " << MIDIManager::getNoteName(pitch) << ", " << velocity);

	if (useGenericControls) updateValue(channel, MIDIManager::getNoteName(pitch), velocity);
	
}

void MIDIModule::controlChangeReceived(const int & channel, const int & number, const int & value)
{
	if (!enabled->boolValue()) return; 
	inActivityTrigger->trigger();
	if (logIncomingData->boolValue()) NLOG(niceName, "Control Change : " << channel << ", " << number << ", " << value);

	if (useGenericControls) updateValue(channel, "CC"+String(number), value);

}

void MIDIModule::updateValue(const int &channel, const String & n, const int & val)
{
	const String nWithChannel = "[" + String(channel) + "] " + n;
	Parameter * p = dynamic_cast<Parameter *>(valuesCC.getControllableByName(nWithChannel,true));
	if (p == nullptr)
	{
		if (autoAdd->boolValue())
		{
			p = new IntParameter(nWithChannel, "Channel "+String(channel)+" : "+n, val, 0, 127);
			p->isRemovableByUser = true;
			p->saveValueOnly = false;
			valuesCC.addParameter(p);
			valuesCC.orderControllablesAlphabetically();
		}
	}
	else
	{
		p->setValue(val);
	}

}

var MIDIModule::getJSONData()
{
	var data = Module::getJSONData();
	data.getDynamicObject()->setProperty("values", valuesCC.getJSONData());
	return data;
}

void MIDIModule::loadJSONDataInternal(var data)
{
	Module::loadJSONDataInternal(data);
	valuesCC.loadJSONData(data.getProperty("values", var()), true);
	valuesCC.orderControllablesAlphabetically();

	setupIOConfiguration(inputDevice != nullptr || valuesCC.controllables.size() > 0, outputDevice != nullptr);
}

/*
InspectableEditor * MIDIModule::getEditor(bool isRoot)
{
	return new MIDIModuleEditor(this,isRoot);
}
*/
