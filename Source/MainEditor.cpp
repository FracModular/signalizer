/*
==============================================================================

This file was auto-generated by the Introjucer!

It contains the basic startup code for a Juce application.

==============================================================================
*/

#include "MainEditor.h"
#include "CVectorScope.h"
//#include "COscilloscope.h"
#include "CSpectrum.h"
#include "SignalizerDesign.h"
#include <cpl/CPresetManager.h>
#include <cpl/LexicalConversion.h>

namespace cpl
{
	const ProgramInfo programInfo
	{
		"Signalizer", 
		"0.1",
		0x001000,
		"Janus Thorborg",
		"sgn",
		false,
		nullptr

	};

};

namespace Signalizer
{
	const static int defaultLength = 700, defaultHeight = 480;
	const static std::vector<std::string> RenderingEnginesList = { "Software", "OpenGL" };

	const static juce::String MainEditorName = "Main Editor Settings";

	const char * ViewIndexToMap[] = 
	{
		"Vectorscope",
		"Oscilloscope",
		"Spectrum",
		"Statistics"
	};

	enum class ViewTypes
	{
		Vectorscope,
		Oscilloscope,
		Spectrum,
		end
	};

	enum class Editors
	{
		GlobalSettings,
		Vectorscope,
		Oscilloscope,
		Spectrum,
		end
	};
	enum class RenderTypes
	{
		Software,
		openGL,
		end
	};

	enum class Utility
	{
		Freeze,
		Sync,
		IdleInBack,
		end

	};
	
	const static std::array<int, 5> AntialisingLevels =
	{
		1,
		2,
		4,
		8,
		16
	};
	
	const static std::vector<std::string> AntialisingStringLevels =
	{
		"1",
		"2",
		"4",
		"8",
		"16"
	};

	MainEditor::MainEditor(SignalizerAudioProcessor * e)
	:
		engine(e),
		AudioProcessorEditor(e),
		CTopView(this),
		rcc(this, this),
		krenderEngine("Rendering Engine", RenderingEnginesList),
		krefreshRate("Refresh Rate"),
		refreshRate(0),
		oldRefreshRate(0),
		unFocused(true), 
		idleInBack(false),
		isEditorVisible(false),
		selTab(0),
		currentView(nullptr),
		kstableFps("Stable FPS"),
		kvsync("Vertical Sync"),
		kioskCoords(-1, -1),
		firstKioskMode(false),
		hasAnyTabBeenSelected(false),
		viewTopCoord(0),
		krefreshState("Reset state"),
		kpresets(this, "main", kpresets.WithDefault),
		kmaxHistorySize("History size")

	{
		setOpaque(true);
		setMinimumSize(50, 50);
		setBounds(0, 0, defaultLength, defaultHeight);
		initUI();

		kpresets.loadDefaultPreset();
		oglc.setContinuousRepainting(false);

	}


	std::unique_ptr<juce::Component> MainEditor::createEditor()
	{
		auto content = new Signalizer::CContentPage();
		content->setName(MainEditorName);
		if (auto page = content->addPage("Settings", "icons/svg/wrench.svg"))
		{
			if (auto section = new Signalizer::CContentPage::MatrixSection())
			{				
				section->addControl(&krefreshRate, 0);
				section->addControl(&kstableFps, 1);
				section->addControl(&kvsync, 2);
				page->addSection(section, "Update");
			}
			if (auto section = new Signalizer::CContentPage::MatrixSection())
			{
				section->addControl(&krenderEngine, 0);

				section->addControl(&kantialias, 1);

				page->addSection(section, "Quality");
			}
			if (auto section = new Signalizer::CContentPage::MatrixSection())
			{
				section->addControl(&krefreshState, 0);
				section->addControl(&kmaxHistorySize, 1);
				page->addSection(section, "Utility");
			}
		}
		if (auto page = content->addPage("Colours", "icons/svg/painting.svg"))
		{
			if (auto section = new Signalizer::CContentPage::MatrixSection())
			{
				int numKnobsPerLine = colourControls.size() / 2;
				int rem = colourControls.size() % 2;

				for (int y = 0; y < 2; ++y)
				{
					for (int x = 0; x < numKnobsPerLine; ++x)
					{
						section->addControl(&colourControls[y * numKnobsPerLine + x], y);
					}
				}
				if (rem)
					section->addControl(&colourControls[colourControls.size() - 1], 0);
				page->addSection(section, "Colour Scheme");
			}
		}
		if (auto page = content->addPage("Presets", "icons/svg/save.svg"))
		{

			if (auto section = new Signalizer::CContentPage::MatrixSection())
			{
				section->addControl(&kpresets, 0);
				page->addSection(section, "Presets");
			}
		}

		return std::unique_ptr<juce::Component>(content);
	}

	void MainEditor::focusGained(FocusChangeType cause)
	{
		if (idleInBack)
		{
			if (unFocused)
			{
				krefreshRate.bSetValue(oldRefreshRate);
			}
		}

		unFocused = false;
	}
	void MainEditor::focusLost(FocusChangeType cause)
	{
		oldRefreshRate = krefreshRate.bGetValue();
		if (idleInBack)
		{
			if (!unFocused)
			{
				krefreshRate.bSetValue(0.5f);
			}
			unFocused = true;
		}
	}

	void MainEditor::onObjectDestruction(const cpl::Utility::DestructionServer<cpl::CBaseControl>::ObjectProxy & destroyedObject)
	{
		// no-op.
	}

	void MainEditor::pushEditor(juce::Component * editor)
	{
		pushEditor(std::unique_ptr<juce::Component>(editor));
		
	}
	void MainEditor::pushEditor(std::unique_ptr<juce::Component> newEditor)
	{
		if (newEditor.get())
			editorStack.push(std::move(newEditor));

		// beware of move construction! newEditor is now invalid!
		if (auto editor = getTopEditor())
		{
			addAndMakeVisible(editor);
			resized();
			repaint();
		}
	}
	juce::Component * MainEditor::getTopEditor() const
	{
		return editorStack.empty() ? nullptr : editorStack.top().get();
	}
	void MainEditor::popEditor()
	{
		if (!editorStack.empty())
		{
			editorStack.pop();

			if(editorStack.empty() && tabs.isOpen())
				tabs.closePanel();
			else
			{
				resized();
				repaint();
			}
		}
	}
	void MainEditor::clearEditors()
	{
		while (!editorStack.empty())
			editorStack.pop();
	}
	cpl::CView * MainEditor::viewFromIndex(std::size_t index)
	{
		auto it = views.end();
		if ((ViewTypes)index < ViewTypes::end)
			it = views.find(ViewIndexToMap[index]);

		return (it != views.end()) ? it->second.get() : nullptr;
	}

	void MainEditor::setRefreshRate(int rate)
	{
		refreshRate = cpl::Math::confineTo(rate, 10, 1000);
		if (kstableFps.bGetValue() > 0.5)
		{
			juce::HighResolutionTimer::startTimer(refreshRate);
		}
		else
		{
			juce::Timer::startTimer(refreshRate);
		}
		if (currentView)
			currentView->setApproximateRefreshRate(refreshRate);

	}
	void MainEditor::resume()
	{
		setRefreshRate(refreshRate);
	}

	void MainEditor::suspend()
	{
		juce::HighResolutionTimer::stopTimer();
		juce::Timer::stopTimer();
	}

	// these handle cases where our component is being thrown off the kiosk mode by (possibly)
	// another JUCE plugin.
	void MainEditor::componentMovedOrResized(Component& component, bool wasMoved, bool wasResized)
	{
		if (&component == currentView->getWindow())
		{
			if (currentView->getIsFullScreen() && component.isOnDesktop())
			{
				if (juce::ComponentPeer * peer = component.getPeer())
				{
					if (!peer->isKioskMode())
					{
						exitFullscreen();
						if (kkiosk.bGetValue() > 0.5)
						{
							kkiosk.bSetInternal(0.0);
						}
					}
				}
			}
		}
	}
	void MainEditor::componentParentHierarchyChanged(Component& component)
	{
		if (&component == currentView->getWindow())
		{
			if (currentView->getIsFullScreen() && component.isOnDesktop())
			{
				if (juce::ComponentPeer * peer = component.getPeer())
				{
					if (!peer->isKioskMode())
					{
						exitFullscreen();
						if (kkiosk.bGetValue() > 0.5)
						{
							kkiosk.bSetInternal(0.0);
						}
					}
				}
			}
		}
	}

	void MainEditor::valueChanged(const cpl::CBaseControl * c)
	{
		// bail out early if we aren't showing anything.
		if (!currentView)
			return;

		auto value = c->bGetValue();

		// freezing of displays
		if (c == &kfreeze)
		{
			if (value > 0.5)
			{
				engine->stream.setSuspendedState(true);
			}
			else
			{
				engine->stream.setSuspendedState(false);
			}
		}
		// syncing of audio stream with views
		else if (c == &ksync)
		{
			if (value > 0.5)
			{
				currentView->setSyncing(true);
			}
			else
			{
				currentView->setSyncing(false);
			}
		}
		// lower display rate if we are unfocused
		else if (c == &kidle)
		{
			idleInBack = value > 0.5 ? true : false;
		}

		else if (c == &ksettings)
		{
			if (value > 0.5)
			{
				// spawn the global setting editor
				tabs.openPanel();
				pushEditor(this->createEditor());
			}
			else
			{
				// -- remove it
				if (auto editor = getTopEditor())
				{
					if (editor->getName() == MainEditorName)
					{
						popEditor();
					}
				}
			}
		}
		else if (c == &kkiosk)
		{
			if (currentView)
			{
				if (value > 0.5 && currentView)
				{
					// set a window to fullscreen.
					if (firstKioskMode)
					{
						// dont fetch the current coords if we set the view the first time..
						firstKioskMode = false;
					}
					else
					{
						if (juce::Desktop::getInstance().getKioskModeComponent() == currentView->getWindow())
							return;
						kioskCoords = currentView->getWindow()->getScreenPosition();
					}

					removeChildComponent(currentView->getWindow());
					currentView->getWindow()->addToDesktop(juce::ComponentPeer::StyleFlags::windowAppearsOnTaskbar);

					currentView->getWindow()->setTopLeftPosition(kioskCoords.x, kioskCoords.y);

					juce::Desktop::getInstance().setKioskModeComponent(currentView->getWindow(), false);
					currentView->setFullScreenMode(true);
					currentView->getWindow()->setWantsKeyboardFocus(true);
					currentView->getWindow()->grabKeyboardFocus();
					// add listeners.

					currentView->getWindow()->addKeyListener(this);
					currentView->getWindow()->addComponentListener(this);

				}
				else
				{
					exitFullscreen();
				}
			}
		}
		else if (c == &kstableFps)
		{
			if (kstableFps.bGetValue() > 0.5)
			{
				juce::Timer::stopTimer();
				juce::HighResolutionTimer::startTimer(refreshRate);
			}
			else
			{
				juce::HighResolutionTimer::stopTimer();
				juce::Timer::startTimer(refreshRate);
			}
		}
		else if (c == &kvsync)
		{
			if (kvsync.bGetValue() > 0.5)
			{
				if (currentView)
				{
					currentView->setSwapInterval(1);

					// this is kind of stupid; the sync setting must be set after the context is created..
					std::function<void(void)> f = [&]() 
					{
						if (oglc.isAttached())
							oglc.setContinuousRepainting(true);
						else
							cpl::GUIUtils::FutureMainEvent(200, f, this);
					};
					f();
				}
			}
			else
			{
				if (currentView)
				{
					currentView->setSwapInterval(-1);
					oglc.setContinuousRepainting(false);
				}
			}
		}
		// change of refresh rate
		else if (c == &krefreshRate)
		{
			refreshRate = cpl::Math::round<int>(cpl::Math::UnityScale::exp(value, 10.0, 1000.0));
			setRefreshRate(refreshRate);
		}
		// change of the rendering engine
		else if (c == &krenderEngine)
		{
			auto index = cpl::distribute<RenderTypes>(value);

			switch (index)
			{
			case RenderTypes::Software:

				if (currentView && currentView->isOpenGL())
				{
					currentView->detachFromOpenGL(oglc);
					if (oglc.isAttached())
						oglc.detach();
				}


				break;
			case RenderTypes::openGL:
				if (oglc.isAttached())
				{
					if (currentView && !currentView->isOpenGL())
					{
						// ?? freaky
						if (cpl::CView * unknownView = dynamic_cast<cpl::CView *>(oglc.getTargetComponent()))
							unknownView->detachFromOpenGL(oglc);
						oglc.detach();
					}
				}
				if (currentView)
					currentView->attachToOpenGL(oglc);
				break;
			}
		}
		else if (c == &kantialias)
		{
			setAntialiasing();
		}
		else if (c == &krefreshState)
		{
			if (currentView)
				currentView->resetState();
		}
		else if (c == &kmaxHistorySize)
		{

			struct RetryResizeCapacity
			{
				RetryResizeCapacity(MainEditor * h) : handle(h) {};
				MainEditor * handle;

				void operator()()
				{
					auto currentSampleRate = handle->engine->stream.getInfo().sampleRate.load(std::memory_order_acquire);
					if (currentSampleRate > 0)
					{
						std::int64_t value;
						std::string contents = handle->kmaxHistorySize.getInputValue();
						if (cpl::lexicalConversion(contents, value) && value >= 0)
						{
							auto msCapacity = cpl::Math::round<std::size_t>(currentSampleRate * 0.001 * value);

							handle->engine->stream.setAudioHistoryCapacity(msCapacity);

							if (contents.find_first_of("ms") == std::string::npos)
							{
								contents.append(" ms");
								handle->kmaxHistorySize.setInputValueInternal(contents);
							}

							handle->kmaxHistorySize.indicateSuccess();
						}
						else
						{
							std::string result;
							auto msCapacity = cpl::Math::round<std::size_t>(1000 * handle->engine->stream.getAudioHistoryCapacity() / handle->engine->stream.getInfo().sampleRate);
							if (cpl::lexicalConversion(msCapacity, result))
								handle->kmaxHistorySize.setInputValueInternal(result + " ms");
							else
								handle->kmaxHistorySize.setInputValueInternal("error");
							handle->kmaxHistorySize.indicateError();
						}
					}
					else
					{
						cpl::GUIUtils::FutureMainEvent(200, RetryResizeCapacity(handle), handle);
					}

				}
			};

			RetryResizeCapacity(this)();

		}
		else
		{
			// check if it was one of the colours
			for (unsigned i = 0; i < colourControls.size(); ++i)
			{
				if (c == &colourControls[i])
				{
					// change colour and broadcast event.
					cpl::CLookAndFeel_CPL::defaultLook().getSchemeColour(i).colour = colourControls[i].getControlColourAsColour();
					cpl::CLookAndFeel_CPL::defaultLook().updateColours();
					repaint();
				}

			}
		}
	}


	void MainEditor::panelOpened(cpl::CTextTabBar<> * obj)
	{
		isEditorVisible = true;
		if (cpl::CView * view = viewFromIndex(selTab))
		{
			pushEditor(view->createEditor());
		}
		resized();
		repaint();
	}
	void MainEditor::panelClosed(cpl::CTextTabBar<> * obj)
	{
		clearEditors();
		ksettings.setToggleState(false, NotificationType::dontSendNotification);
		resized();
		repaint();
	}
	
	void MainEditor::setAntialiasing(int multisamplingLevel)
	{

		int sanitizedLevel = 1;
		
		if(multisamplingLevel == -1)
		{
			auto val = cpl::Math::confineTo(kantialias.getZeroBasedSelIndex(), 0, AntialisingLevels.size() - 1);
			sanitizedLevel = AntialisingLevels[val];
		}
		else
		{
			for(unsigned i = 0; i < AntialisingLevels.size(); ++i)
			{
				if(AntialisingLevels[i] == multisamplingLevel)
				{
					sanitizedLevel = AntialisingLevels[i];
					break;
				}
			}
		}
		
		if(sanitizedLevel > 0)
		{
			OpenGLPixelFormat fmt;
			fmt.multisamplingLevel = sanitizedLevel;
			// true if a view exists and it is attached
			bool reattach = false;
			if(currentView)
			{
				if(currentView->isOpenGL())
				{
					currentView->detachFromOpenGL(oglc);
					reattach = true;
				}
			}
			oglc.setMultisamplingEnabled(true);
			oglc.setPixelFormat(fmt);
			
			if(reattach)
			{
				currentView->attachToOpenGL(oglc);
			}
			
		}
		else
		{
			oglc.setMultisamplingEnabled(false);
		}
		
	}

	int MainEditor::getRenderEngine()
	{
		return (int)cpl::distribute<RenderTypes>(krenderEngine.bGetValue());
	}

	void MainEditor::suspendView(cpl::CView * view)
	{
		if (view)
		{
			if (oglc.isAttached())
			{
				view->detachFromOpenGL(oglc);
				oglc.detach();
			}
			else
			{
				// check if view is still attached to a dead context:
				if(view->isOpenGL())
				{
					// something else must have killed the context, check if it's the same
					
					if(view->getAttachedContext() == &oglc)
					{
						// okay, so we detach it anyway and eat the exceptions:
						view->detachFromOpenGL(oglc);
					}
				}
				
			}
			if (kkiosk.bGetValue() > 0.5 && juce::Desktop::getInstance().getKioskModeComponent() == view->getWindow())
			{
				exitFullscreen();
			}
			removeChildComponent(currentView->getWindow());
			view->getWindow()->removeMouseListener(this);
			view->suspend();
		}

	}

	void MainEditor::initiateView(cpl::CView * view)
	{
		if (view)
		{
			if (view != currentView)
			{
				// trying to add the same window twice without suspending it?
				jassertfalse;
			}
			view->resume();
			currentView = view;
			addAndMakeVisible(view->getWindow());

			if ((RenderTypes)getRenderEngine() == RenderTypes::openGL)
			{
				// init all openGL stuff.
				setAntialiasing();
				view->attachToOpenGL(oglc);
				view->setSwapInterval(kvsync.bGetValue() > 0.5 ? 1 : -1);
			}
			if (kkiosk.bGetValue() > 0.5)
			{
				if (firstKioskMode)
					enterFullscreenIfNeeded(kioskCoords);
				else
					enterFullscreenIfNeeded();
			}
			resized();
			view->getWindow()->addMouseListener(this, true);
		}
		else
		{
			// adding non-existant window?
			jassertfalse;
		}
	}

	void MainEditor::mouseUp(const MouseEvent& event)
	{
		if (currentView && event.eventComponent == currentView->getWindow())
		{
			if (event.mods.testFlags(ModifierKeys::rightButtonModifier))
			{
				kfreeze.setToggleState(!kfreeze.getToggleState(), juce::NotificationType::sendNotification);
			}
		}
		else
		{
			AudioProcessorEditor::mouseUp(event);
		}
	}
	void  MainEditor::mouseDown(const MouseEvent& event)
	{
		if (currentView && event.eventComponent == currentView->getWindow())
		{
			if (event.mods.testFlags(ModifierKeys::rightButtonModifier))
			{
				kfreeze.setToggleState(!kfreeze.getToggleState(), juce::NotificationType::sendNotification);
			}
		}
		else
		{
			AudioProcessorEditor::mouseUp(event);
		}
	}
	void MainEditor::tabSelected(cpl::CTextTabBar<> * obj, int index)
	{

		hasAnyTabBeenSelected = true;
		// these lines disable the global editor if you switch view.
		//if (ksettings.getToggleState())
		//	ksettings.setToggleState(false, NotificationType::sendNotification);

		// see if the new view exists.
		auto const & mappedView = Signalizer::ViewIndexToMap[index];
		auto it = views.find(mappedView);

		cpl::CSubView * view = nullptr;

		if (it == views.end())
		{

			// insert the new view into the map
			switch ((ViewTypes)index)
			{
			case ViewTypes::Vectorscope:
				view = new CVectorScope(engine->stream);
				break;
			case ViewTypes::Oscilloscope:
			//	view = new COscilloscope(engine->audioBuffer);
				break;
			case ViewTypes::Spectrum:
				view = new CSpectrum(engine->stream);
				break;
			default:
				break;
			}
			if (view)
			{
				views.emplace(mappedView, std::unique_ptr<cpl::CSubView>(view));
				auto & key = viewSettings.getKey("Serialized Views").getKey(mappedView);
				if (!key.isEmpty())
					view->load(key, key.getMasterVersion());
			}
			else
			{
				view = &defaultView;
			}
		}
		else
		{
			view = it->second.get();
		}
		// if any editor is open currently, we have to close it and open the new.
		bool openNewEditor = false;
		// deattach old view
		if (currentView)
		{
			if (getTopEditor())
				openNewEditor = true;

			clearEditors();

			suspendView(currentView);
			currentView = nullptr;
		}

		currentView = view;
		if (currentView)
		{
			initiateView(currentView);
			if (openNewEditor)
				pushEditor(currentView->createEditor());

			currentView->setSyncing(ksync.bGetBoolState());
		}
		
		if (openNewEditor && ksettings.bGetBoolState())
			ksettings.bSetInternal(0.0);



		selTab = index;
	}

	void MainEditor::activeTabClicked(cpl::CTextTabBar<>* obj, int index)
	{
		ksettings.bSetValue(0);

	}


	void MainEditor::addTab(const std::string & name)
	{
		tabs.addTab(name);
	}

	void MainEditor::save(cpl::CSerializer & data, long long int version)
	{

		data << krefreshRate;
		data << krenderEngine;
		data << ksync;
		data << kfreeze;
		data << kidle;
		data << getBounds().withZeroOrigin();
		data << isEditorVisible;
		data << selTab;
		data << kioskCoords;
		data << hasAnyTabBeenSelected;
		data << kkiosk;

		// stuff that gets set the last.
		data << kantialias;
		data << kvsync;

		for (auto & colour : colourControls)
		{
			data.getKey("Colours").getKey(colour.bGetTitle()) << colour;
		}

		// save any view data

		// copy old session data

		// walk the list of possible plugins
		for (auto & viewName : ViewIndexToMap)
		{		
			
			auto viewInstanceIt = views.find(viewName);
			// see if they're instantiated, in which case
			if (viewInstanceIt != views.end())
			{
				// serialize fresh data - // watch out, or it'll save the std::unique_ptr!
				data.getKey("Serialized Views").getKey(viewInstanceIt->first) << viewInstanceIt->second.get();
			}
			else
			{
				// otherwise, see if we have some old session data:
				auto serializedView = viewSettings.getKey("Serialized Views").getKey(viewName);
				if (!serializedView.isEmpty())
				{
					data.getKey("Serialized Views").getKey(viewName) = serializedView;
				}
			}
		}

	}

	void MainEditor::enterFullscreenIfNeeded(juce::Point<int> where)
	{
		// full screen set?
		if (kkiosk.bGetValue() > 0.5)
		{

			// avoid storing the current window position into kioskCoords
			// the first time we spawn the view.
			firstKioskMode = true;
			currentView->getWindow()->setTopLeftPosition(where.x, where.y);
			kkiosk.bForceEvent();
		}
	}

	void MainEditor::enterFullscreenIfNeeded()
	{
		// full screen set?
		if (currentView && !currentView->getIsFullScreen() && kkiosk.bGetValue() > 0.5)
		{
			// avoid storing the current window position into kioskCoords
			// the first time we spawn the view.
			firstKioskMode = false;
			kkiosk.bForceEvent();
		}
	}

	void MainEditor::exitFullscreen()
	{
		juce::Desktop & instance = juce::Desktop::getInstance();
		if (currentView && currentView->getIsFullScreen() && !this->isParentOf(currentView->getWindow()))
		{
			currentView->getWindow()->removeKeyListener(this);
			currentView->getWindow()->removeComponentListener(this);
			if (currentView->getWindow() == instance.getKioskModeComponent())
			{
				juce::Desktop::getInstance().setKioskModeComponent(nullptr);
			}

			currentView->getWindow()->setTopLeftPosition(0, 0);
			addChildComponent(currentView->getWindow());
			currentView->setFullScreenMode(false);
			resized();

		}

	}

	void MainEditor::load(cpl::CSerializer & data, long long version)
	{
		viewSettings = data;
		//cpl::iCtrlPrec_t dataVal(0);
		juce::Rectangle<int> bounds;

		data >> krefreshRate;
		data >> krenderEngine;
		data >> ksync;
		data >> kfreeze;
		data >> kidle;
		data >> bounds;
		data >> isEditorVisible;
		data >> selTab;
		data >> kioskCoords;
		data >> hasAnyTabBeenSelected;
		// kind of a hack, but we don't really want to enter kiosk mode immediately.
		kkiosk.bRemovePassiveChangeListener(this);
		data >> kkiosk;
		kkiosk.bAddPassiveChangeListener(this);
		for (auto & colour : colourControls)
		{
			auto & content = viewSettings.getKey("Colours").getKey(colour.bGetTitle());
			if (!content.isEmpty())
				content >> colour;
		}

		// sanitize bounds...
		setBounds(bounds.constrainedWithin(
			juce::Desktop::getInstance().getDisplays().getDisplayContaining(bounds.getPosition()).userArea
		).withZeroOrigin());

		// reinitiaty any current views (will not be done through tab selection further down)
		for (auto & viewPair : views)
		{
			auto & key = viewSettings.getKey("Serialized Views").getKey(viewPair.first);
			if (!key.isEmpty())
				viewPair.second->load(key, key.getMasterVersion());
		}

		// will take care of opening the correct view
		if (hasAnyTabBeenSelected)
		{
			// settings this will cause setSelectedTab to 
			// enter fullscreen at kioskCoords
			if (kkiosk.bGetValue() > 0.5)
				firstKioskMode = true;
			tabs.setSelectedTab(selTab);
		}
		else
		{
			// if not, make sure we entered full screen if needed.
			enterFullscreenIfNeeded(kioskCoords);
		}


		// set this kind of stuff after view is initiated. note, these cause events to be fired!
		data >> kantialias;
		data >> kvsync;


	}

	bool MainEditor::stringToValue(const cpl::CBaseControl * ctrl, const std::string & valString, cpl::iCtrlPrec_t & val)
	{
		if (ctrl == &krefreshRate)
		{
			char * endPtr = nullptr;
			auto newVal = strtod(valString.c_str(), &endPtr);
			if (endPtr > valString.c_str())
			{
				newVal = cpl::Math::confineTo
				(
					cpl::Math::UnityScale::Inv::exp
					(
						cpl::Math::confineTo(newVal, 10.0, 1000.0), 
						10.0, 
						1000.0
					),
					0.0,
					1.0
				);
				val = newVal;
				return true;
			}
		}
		return false;
	}

	bool MainEditor::valueToString(const cpl::CBaseControl * ctrl, std::string & valString, cpl::iCtrlPrec_t val)
	{
		if (ctrl == &krefreshRate)
		{
			auto refreshRateVal = cpl::Math::round<int>(cpl::Math::UnityScale::exp(val, 10.0, 1000.0));
			valString = std::to_string(refreshRateVal) + " ms";
			return true;
		}
		return false;
	}

	bool MainEditor::keyPressed(const KeyPress &key, Component *originatingComponent)
	{
		if (currentView && (currentView->getWindow() == originatingComponent))
		{
			if (key.isKeyCode(key.escapeKey) && kkiosk.bGetValue() > 0.5)
			{
				exitFullscreen();

				kkiosk.bSetInternal(0.0);
				return true;
			}
		}
		return false;
	}
	MainEditor::~MainEditor()
	{
		notifyDestruction();
		suspendView(currentView);
		exitFullscreen();
		juce::Timer::stopTimer();
		juce::HighResolutionTimer::stopTimer();

	}
	void MainEditor::resizeEnd()
	{
		resized();
		resume();
	}

	void MainEditor::resizeStart()
	{
		suspend();
	}

	void MainEditor::resized()
	{
		auto const elementSize = 25;
		auto const elementBorder = 1; // border around all elements, from which the background shines through
		auto const buttonSize = elementSize - elementBorder * 2;
		auto const buttonSizeW = elementSize - elementBorder * 2;
		rcc.setBounds(getWidth() - 15, getHeight() - 15, 15, 15);
		// dont resize while user is dragging
		if (rcc.isMouseButtonDown())
			return;
		// resize panel to width

		auto const width = getWidth();
		auto leftBorder = width - elementSize + elementBorder;
		ksettings.setBounds(1, 1, buttonSizeW, buttonSize);

		kfreeze.setBounds(leftBorder, 1, buttonSizeW, buttonSize);
		leftBorder -= elementSize - elementBorder;
		// TODO: erase ksync entirely
		/*ksync.setBounds(leftBorder, 1, buttonSize, buttonSize);
		leftBorder -= elementSize - elementBorder;*/
		kidle.setBounds(leftBorder, 1, buttonSizeW, buttonSize);
		leftBorder -= elementSize - elementBorder;
		kkiosk.setBounds(leftBorder, 1, buttonSizeW, buttonSize);
		tabs.setBounds
		(
			ksettings.getBounds().getRight() + elementBorder,
			elementBorder,
			getWidth() - (ksettings.getWidth() + getWidth() - leftBorder + elementBorder * 3),
			elementSize - elementBorder * 2
		 );


		/*rightButtonOutlines.clear();
		rightButtonOutlines.addLineSegment(juce::Line<float>(ksettings.getRight(), 1.f, ksettings.getRight(), (float)elementSize - 1), 0.1f);
		// add a line underneath to seperate..
		if (ksettings.bGetValue() > 0.1f)
			rightButtonOutlines.addLineSegment(juce::Line<float>(1, ksettings.getBottom() - 0.2f, ksettings.getRight(), ksettings.getBottom() - 0.2f), 0.1f);
		rightButtonOutlines.addLineSegment(juce::Line<float>(tabs.getRight(), 1.f, tabs.getRight(), (float)elementSize - 1), 0.1f);
		rightButtonOutlines.addLineSegment(juce::Line<float>(kidle.getRight(), 1.f, kidle.getRight(), (float)elementSize - 1), 0.1f);
		rightButtonOutlines.addLineSegment(juce::Line<float>(ksync.getRight(), 1.f, ksync.getRight(), (float)elementSize - 1), 0.1f);
		*/
		//rightButtonOutlines.addRectangle(ksettings.getBounds());
		//rightButtonOutlines.addRectangle(tabs.getBounds());
		//rightButtonOutlines.addRectangle(juce::Rectangle<int>(kidle.getX(), 0, elementSize * 3, elementSize));


		auto editor = getTopEditor();
		if (editor)
		{
			auto maxHeight = elementSize * 5;

			// content pages knows their own (dynamic) size.
			if (auto signalizerEditor = dynamic_cast<Signalizer::CContentPage *>(editor))
			{
				maxHeight = std::max(0, std::min(maxHeight, signalizerEditor->getSuggestedSize().second));
			}
			editor->setBounds(elementBorder, tabs.getBottom(), getWidth() - elementBorder * 2, maxHeight);
			viewTopCoord = tabs.getHeight() + editor->getHeight() + elementBorder;
		}
		else
		{
			viewTopCoord = tabs.getBottom() + elementBorder;
		}
		// full screen components resize themselves.
		if (currentView && !currentView->getIsFullScreen())
		{
			currentView->getWindow()->setBounds(0, viewTopCoord, getWidth(), getHeight() - viewTopCoord);
		}

		//rightButtonOutlines.addRectangle(juce::Rectangle<float>(0.5f, 0.5f, getWidth() - 1.5f, editor ? editor->getBottom() : elementSize - 1.5f));
	}


	void MainEditor::timerCallback()
	{
		
		if (currentView)
		{
			//const MessageManagerLock mml;

			if (idleInBack)
			{
				if (!hasKeyboardFocus(true))
					focusLost(FocusChangeType::focusChangedDirectly);
				else if (unFocused)
					focusGained(FocusChangeType::focusChangedDirectly);
			}

			currentView->repaintMainContent();
		}
	}
	void MainEditor::hiResTimerCallback()
	{
		if (currentView)
		{
			

			if (idleInBack)
			{
				const MessageManagerLock mml;
				if (!hasKeyboardFocus(true))
					focusLost(FocusChangeType::focusChangedDirectly);
				else if (unFocused)
					focusGained(FocusChangeType::focusChangedDirectly);
			}

			currentView->repaintMainContent();
		}

	}



	//==============================================================================
	void MainEditor::paint(Graphics& g)
	{
		// make sure to paint everything completely opaque.
		g.setColour(cpl::GetColour(cpl::ColourEntry::separator).withAlpha(1.0f));
		g.fillRect(getBounds().withZeroOrigin().withBottom(viewTopCoord));
		if (kkiosk.bGetValue() > 0.5)
		{
			g.setColour(cpl::GetColour(cpl::ColourEntry::deactivated).withAlpha(1.0f));
			g.fillRect(getBounds().withZeroOrigin().withTop(viewTopCoord));
			g.setColour(cpl::GetColour(cpl::ColourEntry::auxfont));
			g.drawText("View is fullscreen", getBounds().withZeroOrigin().withTop(viewTopCoord), juce::Justification::centred, true);
		}
	}


	void MainEditor::initUI()
	{
		auto & lnf = cpl::CLookAndFeel_CPL::defaultLook();
		// add listeners
		krefreshRate.bAddFormatter(this);
		kfreeze.bAddPassiveChangeListener(this);
		kkiosk.bAddPassiveChangeListener(this);
		kidle.bAddPassiveChangeListener(this);
		krenderEngine.bAddPassiveChangeListener(this);
		krefreshRate.bAddPassiveChangeListener(this);
		ksettings.bAddPassiveChangeListener(this);
		kstableFps.bAddPassiveChangeListener(this);
		kvsync.bAddPassiveChangeListener(this);
		tabs.addListener(this);
		kmaxHistorySize.bAddPassiveChangeListener(this);

		kantialias.bAddPassiveChangeListener(this);
		ksync.bAddPassiveChangeListener(this);
		krefreshState.bAddPassiveChangeListener(this);
		// design
		kfreeze.setImage("icons/svg/snow1.svg");
		kidle.setImage("icons/svg/idle.svg");
		ksettings.setImage("icons/svg/gears.svg");
		ksync.setImage("icons/svg/sync2.svg");
		kkiosk.setImage("icons/svg/fullscreen.svg");

		kstableFps.setSize(cpl::ControlSize::Rectangle.width, cpl::ControlSize::Rectangle.height / 2);
		kvsync.setSize(cpl::ControlSize::Rectangle.width, cpl::ControlSize::Rectangle.height / 2);
		krefreshState.setSize(cpl::ControlSize::Rectangle.width, cpl::ControlSize::Rectangle.height / 2);
		kstableFps.setToggleable(true);
		kvsync.setToggleable(true);
		kantialias.bSetTitle("Antialiasing");
		// setup
		krenderEngine.setValues(RenderingEnginesList);
		kantialias.setValues(AntialisingStringLevels);

		// initiate colours
		for (unsigned i = 0; i < colourControls.size(); ++i)
		{
			auto & schemeColour = lnf.getSchemeColour(i);
			colourControls[i].setControlColour(schemeColour.colour.getPixelARGB());
			colourControls[i].bSetTitle(schemeColour.name);
			colourControls[i].bSetDescription(schemeColour.description);
			colourControls[i].bAddPassiveChangeListener(this);

		}

		// add stuff
		addAndMakeVisible(ksettings);
		addAndMakeVisible(kfreeze);
		// TODO: erase ksync
		//addAndMakeVisible(ksync);
		addAndMakeVisible(kkiosk);
		addAndMakeVisible(tabs);
		addAndMakeVisible(kidle);
		tabs.setOrientation(tabs.Horizontal);
		tabs.addTab("VectorScope").addTab("Oscilloscope").addTab("Spectrum").addTab("Statistics");

		// additions
		addAndMakeVisible(rcc);
		rcc.setAlwaysOnTop(true);
		currentView = &defaultView; // note: enables callbacks on value set in this function
		addAndMakeVisible(defaultView);
		krefreshRate.bSetValue(0.12);


		// descriptions
		kstableFps.bSetDescription("Stabilize frame rate using a high precision timer.");
		kvsync.bSetDescription("Synchronizes graphic view rendering to your monitors refresh rate.");
		kantialias.bSetDescription("Set the level of hardware antialising applied.");
		krefreshRate.bSetDescription("How often the view is redrawn.");
		ksync.bSetDescription("Synchronizes audio streams with view drawing; may incur buffer underruns for low settings.");
		kkiosk.bSetDescription("Puts the view into fullscreen mode. Press Escape to untoggle, or tab out of the view.");
		kidle.bSetDescription("If set, lowers the frame rate of the view if this plugin is not in the front.");
		ksettings.bSetDescription("Open the global settings for the plugin (presets, themes and graphics).");
		kfreeze.bSetDescription("Stops the view from updating, allowing you to examine the current point in time.");
		krefreshState.bSetDescription("Resets any processing state in the active view to the default.");
		kmaxHistorySize.bSetDescription("The maximum audio history capacity, set in the respective views. No limit, so be careful!");
		
		// initial values that should be through handlers
		kmaxHistorySize.setInputValue("1000");

		
		resized();
	}
};