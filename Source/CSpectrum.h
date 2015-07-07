
#ifndef _CSPECTRUM_H
	#define _CSPECTRUM_H

	#include <cpl/Common.h>
	#include <cpl/CAudioBuffer.h>
	#include <cpl/CViews.h>
	#include <cpl/GraphicComponents.h>
	#include <cpl/Utility.h>
	#include <cpl/gui/Controls.h>
	#include <cpl/dsp/CComplexResonator.h>
	#include <memory>
	#include <cpl/rendering/COpenGLImage.h>
	#include <cpl/CFrequencyGraph.h>
	#include <cpl/dsp/CPeakFilter.h>
	#include <cpl/CDBMeterGraph.h>

	namespace cpl
	{
		namespace OpenGLEngine
		{
			class COpenGLStack;
			
		};
	};


	namespace Signalizer
	{
	
		class SFrameBuffer
		{



		};


		template<typename Scalar>
		union UComplexFilter
		{
			UComplexFilter() : real(0), imag(0) { }
			struct
			{
				Scalar real, imag;
			};
			struct
			{
				Scalar magnitude, phase;
			};
			struct
			{
				Scalar leftMagnitude, rightMagnitude;
			};
		};



		
		class CSpectrum
		: 
			public cpl::COpenGLView, 
			protected cpl::CBaseControl::PassiveListener,
			protected cpl::CBaseControl::ValueFormatter,
			protected cpl::CBaseControl::Listener,
			protected cpl::CAudioListener,
			protected juce::ComponentListener,
			public cpl::Utility::CNoncopyable
		{

		public:

			typedef float fpoint;

			enum class DisplayMode
			{
				LineGraph,
				ColourSpectrum
			};

			enum class ChannelConfiguration
			{
				/// <summary>
				/// Only the left channel will be analyzed and displayed.
				/// </summary>
				Left, 
				/// <summary>
				/// Only the right channel will be analyzed and displayed.
				/// </summary>
				Right, 
				/// <summary>
				/// Left and right will be merged (added) together and processed
				/// in mono mode
				/// </summary>
				Merge, 
				/// <summary>
				/// Both channels will be processed seperately, and the average of the magnitude will be displayed,
				/// together with a graph of the scaled phase cancellation.
				/// </summary>
				Phase, 
				/// <summary>
				/// Both channels are displayed.
				/// </summary>
				Separate
			};

			enum class TransformAlgorithm
			{
				FFT, RSNT
			};

			enum class ViewScaling
			{
				Linear,
				Logarithmic
			};

			

			struct DBRange
			{
				double low, high;
			};

			CSpectrum(cpl::AudioBuffer & data);
			virtual ~CSpectrum();

			// Component overrides
			void onGraphicsRendering(Graphics & g) override;
			void mouseWheelMove(const MouseEvent& event, const MouseWheelDetails& wheel) override;
			void mouseDoubleClick(const MouseEvent& event) override;
			void mouseDrag(const MouseEvent& event) override;
			void mouseUp(const MouseEvent& event) override;
			void mouseDown(const MouseEvent& event) override;
			void resized() override;


			// OpenGLRender overrides
			void onOpenGLRendering() override;
			void initOpenGL() override;
			void closeOpenGL() override;

			// View overrides
			void suspend() override;
			void resume() override;
			void freeze() override;
			void unfreeze() override;

			std::unique_ptr<juce::Component> createEditor() override;
			// CSerializer overrides
			void load(cpl::CSerializer::Builder & builder, long long int version) override;
			void save(cpl::CSerializer::Archiver & archive, long long int version) override;

			// cbasecontrol overrides
			void valueChanged(const cpl::CBaseControl *) override;
			bool valueChanged(cpl::CBaseControl *) override;
			bool stringToValue(const cpl::CBaseControl * ctrl, const std::string & buffer, cpl::iCtrlPrec_t & value) override;
			bool valueToString(const cpl::CBaseControl * ctrl, std::string & buffer, cpl::iCtrlPrec_t value) override;
			void onObjectDestruction(const cpl::CBaseControl::ObjectProxy & destroyedObject) override;
			bool isEditorOpen() const;

			// interface


			void setDBs(double low, double high, bool updateControls = false);
			void setDBs(DBRange &, bool updateControls = false);
			DBRange getDBs() const noexcept;
			std::size_t getWindowSize() const noexcept;
			void setWindowSize(std::size_t size);

		protected:

			bool audioCallback(cpl::CAudioSource & source, float ** buffer, std::size_t numChannels, std::size_t numSamples) override;
			void componentBeingDeleted(Component & 	component) override;

			/// <summary>
			/// Tunes the system accordingly to the view and current zoom.
			/// </summary>
			void mapFrequencies();
			/// <summary>
			/// Maps the current resonating system according to the current model (linear/logarithmic) and the current
			/// subsection of the complete spectrum such that a linear array of output data matches pixels 1:1, as well as 
			/// formats the data into the filterResults array according to the channel mode (ChannelConfiguration).
			/// 
			/// Call prepareTransform(), then doTransform(), then mapToLinearSpace()
			/// </summary>
			void mapToLinearSpace();
			
			/// <summary>
			/// Call this when something affects the view scaling, view size, mapping of frequencies, display modes etc.
			/// Set state accordingly first.
			/// </summary>
			void displayReordered();

			/// <summary>
			/// For some transform algorithms, it may be a no-op, but for others (like FFTs) that may need zero-padding
			/// or windowing, this is done here.
			/// 
			/// Call prepareTransform(), then doTransform(), then mapToLinearSpace()
			/// </summary>
			void prepareTransform();

			/// <summary>
			/// Again, some algorithms may not need this, but this ensures the transform is done after this call.
			/// 
			/// Call prepareTransform(), then doTransform(), then mapToLinearSpace()
			/// </summary>
			void doTransform();
		private:

			/// <summary>
			/// Returns the samplerate of the currently connected channels.
			/// </summary>
			/// <returns></returns>
			float getSampleRate() const noexcept;
			/// <summary>
			/// The number of filters used for algoritms not based on 2^n ffts
			/// This number is generally in relation to the number of pixels on the axis.
			/// </summary>
			int getNumFilters() const noexcept;
			/// <summary>
			/// The number of pixels/points in the frequency axis.
			/// </summary>
			/// <returns></returns>
			int getAxisPoints() const noexcept;
			/// <summary>
			/// Inits the UI.
			/// </summary>
			void initPanelAndControls();
			/// <summary>
			/// Handles audio re-allocations, tunings and such on the correct thread (you)
			/// Call this before any graphic rendering. If openGL is enabled, this MUST be called on the
			/// openGL thread.
			/// Every rendering-state change is handled in here to minimize duplicate heavy changes/resizes
			/// (certain operations imply others)
			/// </summary>
			void handleFlagUpdates();
			/// <summary>
			/// Computes the time-domain window kernel (see state.windowKernel) of getWindowSize() size.
			/// </summary>
			void computeWindowKernel();

			template<typename T>
				T * getAudioMemory();

			template<typename T>
				std::size_t getNumAudioElements() const noexcept;

			template<typename T>
				T * getWorkingMemory();

			template<typename T>
				std::size_t getNumWorkingElements() const noexcept;

			template<typename V>
				void renderColourSpectrum(cpl::OpenGLEngine::COpenGLStack &);

			template<typename V>
				void renderLineGraph(cpl::OpenGLEngine::COpenGLStack &);

			template<typename V>
				void audioProcessing(float ** buffer, std::size_t numChannels, std::size_t numSamples);

			// vars

			struct StateOptions
			{
				bool isEditorOpen, isFrozen, isSuspended;
				bool antialias;
				bool isLinear;
				/// <summary>
				/// Is the spectrum a horizontal device (line graph)
				/// or a vertical coloured spectrum?
				/// </summary>
				DisplayMode displayMode;

				/// <summary>
				/// The current selected algorithm that will digest the audio data into the display.
				/// </summary>
				TransformAlgorithm algo;
				/// <summary>
				/// How the incoming data is interpreted, channel-wise.
				/// </summary>
				ChannelConfiguration configuration;

				ViewScaling viewScale;

				float primitiveSize;
				juce::Colour colourBackground;

				/// <summary>
				/// Describes the lower and higher limit of the dynamic range of the display.
				/// </summary>
				DBRange dynRange;

				/// <summary>
				/// colourOne & two = colours for the main line graphs.
				/// graphColour = colour for the frequency & db grid.
				/// </summary>
				juce::Colour colourOne, colourTwo, gridColour;

				/// <summary>
				/// The window size..
				/// </summary>
				std::size_t windowSize;

				/// <summary>
				/// The current view of the spectrum (might be zoomed, etc.)
				/// </summary>
				cpl::Utility::Bounds<double> viewRect;

				/// <summary>
				/// Logarithmic displays has to start at a positive value.
				/// </summary>
				double minLogFreq;


				cpl::dsp::WindowTypes dspWindow;

			} state;
			
		
			/// <summary>
			/// Set these flags and their status will be handled in the next handleFlagUpdates() call, which
			/// shall be called before any graphic rendering
			/// </summary>
			struct Flags
			{
				volatile bool
					/// <summary>
					/// Dont set this! Set by the flag handler to assert state
					/// </summary>
					internalFlagHandlerRunning,
					/// <summary>
					/// Set this to resize the audio windows (like, when the audio window size (as in fft size) is changed
					/// </summary>
					audioWindowResize,
					/// <summary>
					/// 
					/// </summary>
					audioMemoryResize,
					workingMemoryResize,
					/// <summary>
					/// Set this when resizing the window
					/// </summary>
					resized,
					/// <summary>
					/// Set this when the view is changed, zoomed, whatever. Implicitly set by resized
					/// </summary>
					viewChanged,
					/// <summary>
					/// Set this to affect purely visual changes to the graph (like divisions and such)
					/// </summary>
					frequencyGraphChange,
					/// <summary>
					/// Recomputes the window
					/// </summary>
					windowKernelChange,
					/// <summary>
					/// This flag will be true the first time handleFlagUpdates() is called
					/// </summary>
					firstChange;
			} flags;

			/// <summary>
			/// GUI elements
			/// </summary>
			juce::Component * editor;
			cpl::CComboBox kviewScaling, kalgorithm, kchannelConfiguration, kdisplayMode, kdspWindow;
			cpl::CKnobSlider klowDbs, khighDbs, kdecayRate, kwindowSize, kpctForDivision;
			cpl::CColourControl kline1Colour, kline2Colour, kgridColour;


			/// <summary>
			/// visual objects
			/// </summary>
			cpl::CButton kdiagnostics;
			juce::MouseCursor displayCursor;
			cpl::OpenGLEngine::COpenGLImage oglImage;
			cpl::CFrequencyGraph frequencyGraph;
			//cpl::CDBMeterGraph dbGraph;
			cpl::CBoxFilter<double, 60> avgFps;
			

			// non-state variables
			unsigned long long processorSpeed; // clocks / sec
			juce::Point<float> lastMousePos;
			std::vector<std::unique_ptr<juce::OpenGLTexture>> textures;
			long long lastFrameTick, renderCycles;
			bool wasResized, isSuspended;
			int framePixelPosition;
			std::vector<unsigned char> columnUpdate;
			// dsp objects
			/// <summary>
			/// The complex resonator used for iir spectrums
			/// </summary>
			cpl::dsp::CComplexResonator<fpoint, 2, 3> cresonator;
			/// <summary>
			/// An array, of numFilters size, with each element being the frequency for the filter of 
			/// the corresponding logical display pixel unit.
			/// </summary>
			std::vector<fpoint> mappedFrequencies;
			/// <summary>
			/// The connected, incoming stream of data.
			/// </summary>
			cpl::AudioBuffer & audioStream;
			/// <summary>
			/// Our internal, (frozen) copy of the incoming stream data.
			/// </summary>
			cpl::AudioBuffer audioStreamCopy;
			/// <summary>
			/// The peak filter coefficient, describing the decay rate of the filters.
			/// </summary>
			cpl::CPeakFilter<float> peakFilter;
			/// <summary>
			/// The'raw' formatted state output of the mapped transform algorithms.
			/// Resized in displayReordered
			/// </summary>
			cpl::aligned_vector<UComplexFilter<fpoint>, 32> filterStates;
			/// <summary>
			/// The decay/peak-filtered and scaled outputs of the transforms,
			/// with each element corrosponding to a complex output pixel of getAxisPoints() size.
			/// Resized in displayReordered
			/// </summary>
			cpl::aligned_vector<UComplexFilter<fpoint>, 32> filterResults;
			/// <summary>
			/// Temporary memory buffer for audio applications. Resized in setWindowSize (since the size is a function of the window size)
			/// </summary>
			cpl::aligned_vector<char, 32> audioMemory;
			/// <summary>
			/// Temporary memory buffer for other applications.
			/// Resized in displayReordered
			/// </summary>
			cpl::aligned_vector<char, 32> workingMemory;
			/// <summary>
			/// The time-domain representation of the dsp-window applied to fourier transforms.
			/// </summary>
			cpl::aligned_vector<double, 32> windowKernel;
		};
	
	};

#endif