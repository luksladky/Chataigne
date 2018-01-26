/*
  ==============================================================================

    CustomOSCCommandArgumentManager.cpp
    Created: 22 Feb 2017 8:51:39am
    Author:  Ben

  ==============================================================================
*/

#include "CustomOSCCommandArgumentManager.h"

#include "CustomOSCCommandArgumentManagerEditor.h"

CustomOSCCommandArgumentManager::CustomOSCCommandArgumentManager(bool _mappingEnabled) :
	BaseManager("arguments"),
	mappingEnabled(_mappingEnabled)
{
	selectItemWhenCreated = false;
	editorCanBeCollapsed = false;
	
}

CustomOSCCommandArgument * CustomOSCCommandArgumentManager::addItemWithParam(Parameter * p, var data, bool fromUndoableAction)
{
	CustomOSCCommandArgument * a = new CustomOSCCommandArgument("#" + String(items.size() + 1), p,mappingEnabled);
	a->addArgumentListener(this);
	if (mappingEnabled && items.size() == 1) a->useForMapping->setValue(true); 
	addItem(a, data, fromUndoableAction);
	return a;
}

CustomOSCCommandArgument * CustomOSCCommandArgumentManager::addItemFromType(Parameter::Type type, var data, bool fromUndoableAction)
{
	Parameter * p = nullptr;
	String id = String(items.size() + 1);

	switch (type)
	{
	case Parameter::STRING:
		p = new StringParameter("#" + id, "Argument #" + id + ", type int", "example");
		break;
	case Parameter::FLOAT:
		p = new FloatParameter("#" + id, "Argument #" + id + ", type foat", 0, 0, 1);
		break;
	case Parameter::INT:
		p = new IntParameter("#" + id, "Argument #" + id + ", type int", 0, -1000, 1000);
		break;
	case Parameter::BOOL:
		p = new BoolParameter("#" + id, "Argument #" + id + ", type bool", false);
		break;

        default:
            break;
	}

	if (p == nullptr) return nullptr;
	return addItemWithParam(p, data, fromUndoableAction);
}

CustomOSCCommandArgument * CustomOSCCommandArgumentManager::addItemFromData(var data, bool fromUndoableAction)
{
	String s = data.getProperty("type", "");
	if (s.isEmpty()) return nullptr;
	Parameter * p = (Parameter *)ControllableFactory::createControllable(s);
	if (p == nullptr)
	{
		LOG("Error loading custom argument !");
		return nullptr;
	}

 	return addItemWithParam(p, data, fromUndoableAction);
}

void CustomOSCCommandArgumentManager::autoRenameItems()
{
	for (int i = 0; i < items.size(); i++)
	{
		if (items[i]->niceName.startsWithChar('#')) items[i]->setNiceName("#" + String(i + 1));
	}
}

void CustomOSCCommandArgumentManager::removeItemInternal(CustomOSCCommandArgument * i)
{
	autoRenameItems();
	useForMappingChanged(i);
}

void CustomOSCCommandArgumentManager::useForMappingChanged(CustomOSCCommandArgument * i)
{
	/*
	if (i->useForMapping->boolValue())
	{
		for (auto &it : items) if (it != i) it->useForMapping->setValue(false);
	}
	*/

	argumentManagerListeners.call(&ManagerListener::useForMappingChanged, i);
}

InspectableEditor * CustomOSCCommandArgumentManager::getEditor(bool isRoot)
{
	return new CustomOSCCommandArgumentManagerEditor(this, isRoot);
}
