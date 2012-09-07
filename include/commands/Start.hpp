// Copyright (c) 2010 Ryan Seal <rlseal -at- gmail.com>
//
// This file is part of GnuRadar Software.
//
// GnuRadar is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//  
// GnuRadar is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with GnuRadar.  If not, see <http://www.gnu.org/licenses/>.
#ifndef START_HPP
#define START_HPP

#include <vector>

#include<boost/shared_ptr.hpp>
#include<boost/scoped_ptr.hpp>
#include<boost/filesystem.hpp>
#include<boost/asio.hpp>
#include<boost/lexical_cast.hpp>

#include<hdf5r/HDF5.hpp>

#include<gnuradar/GnuRadarCommand.hpp>
#include<gnuradar/ProducerConsumerModel.h>
#include<gnuradar/Device.h>
#include<gnuradar/GnuRadarDevice.h>
#include<gnuradar/xml/XmlPacket.hpp>
#include<gnuradar/SynchronizedBufferManager.hpp>
#include<gnuradar/xml/SharedBufferHeader.hpp>
#include<gnuradar/SharedMemory.h>
#include<gnuradar/Constants.hpp>
#include<gnuradar/DataParameters.hpp>
#include<gnuradar/network/StatusServer.hpp>
#include<gnuradar/commands/Response.pb.h>
#include<gnuradar/commands/Control.pb.h>
#include<gnuradar/utils/GrHelper.hpp>


namespace gnuradar {
   namespace command {

      class Start : public GnuRadarCommand {

         typedef boost::asio::io_service IoService;
         typedef boost::shared_ptr<SharedMemory> SharedBufferPtr;
         typedef std::vector<SharedBufferPtr> SharedArray;
         typedef boost::shared_ptr<HDF5> Hdf5Ptr;
         typedef boost::shared_ptr<SynchronizedBufferManager> SynchronizedBufferManagerPtr;
         typedef boost::shared_ptr< ProducerConsumerModel > PCModelPtr;
         typedef boost::shared_ptr< ProducerThread > ProducerThreadPtr;
         typedef boost::shared_ptr< ConsumerThread > ConsumerThreadPtr;
         typedef boost::shared_ptr< GnuRadarDevice > GnuRadarDevicePtr;
         typedef boost::shared_ptr< GnuRadarSettings > GnuRadarSettingsPtr;
         typedef boost::shared_ptr< Device > DevicePtr;
         typedef boost::shared_ptr< ::xml::SharedBufferHeader > SharedBufferHeaderPtr;
         typedef boost::shared_ptr< network::StatusServer > StatusServerPtr;


         // setup shared pointers to extend life beyond this call
         PCModelPtr pcModel_;
         ProducerThreadPtr producer_;
         ConsumerThreadPtr consumer_;
         SynchronizedBufferManagerPtr bufferManager_;
         Hdf5Ptr hdf5_;
         SharedArray array_;
         SharedBufferHeaderPtr header_;
         StatusServerPtr statusServer_;

         /////////////////////////////////////////////////////////////////////////////
         /////////////////////////////////////////////////////////////////////////////
         void CheckForExistingFileSet ( const std::string& fileSet ) throw( std::runtime_error )
         {
            std::string fileName = fileSet + "." + hdf5::FILE_EXT;
            boost::filesystem::path file ( fileName );

            if ( boost::filesystem::exists ( file ) ) {
               throw std::runtime_error( "HDF5 File set " + fileName + 
                     " exists and cannot be overwritten. Change your "
                     "base file set name and try again");
            }
         }

         /////////////////////////////////////////////////////////////////////////////
         /////////////////////////////////////////////////////////////////////////////
         void CreateSharedBuffers( const int bytesPerBuffer ) {

            // setup shared memory buffers
            for ( int i = 0; i < constants::NUM_BUFFERS; ++i ) {

               // create unique buffer file names
               std::string bufferName = constants::BUFFER_BASE_NAME +
                  boost::lexical_cast<string> ( i ) + ".buf";

               // create shared buffers
               SharedBufferPtr bufPtr (
                     new SharedMemory (
                        bufferName,
                        bytesPerBuffer,
                        SHM::CreateShared,
                        0666 )
                     );

               // store buffer in a vector
               array_.push_back ( bufPtr );
            }
         }

         /////////////////////////////////////////////////////////////////////////////
         // pull settings from the configuration file
         /////////////////////////////////////////////////////////////////////////////
         GnuRadarSettingsPtr GetSettings( gnuradar::File* file ) {

            GnuRadarSettingsPtr settings( new GnuRadarSettings() );

            settings->numChannels    = file->channel_size();
            settings->decimationRate = file->decimation();
            settings->fpgaFileName   = file->fpgaimage();

            //Program GNURadio
            for ( int i = 0; i < file->channel_size(); ++i ) {
               settings->Tune ( i, file->channel(i).frequency() );
               settings->Phase( i, file->channel(i).phase() );
            }

            //change these as needed
            settings->fUsbBlockSize  = 0;
            settings->fUsbNblocks    = 0;
            settings->mux            = 0xf3f2f1f0;

            return settings;
         }

         /////////////////////////////////////////////////////////////////////////////
         /////////////////////////////////////////////////////////////////////////////
         Hdf5Ptr SetupHDF5( gnuradar::File* file ) throw( H5::Exception )
         {

            Hdf5Ptr h5File_( new HDF5 ( file->basefilename() , hdf5::WRITE ) );

            h5File_->Description ( "GnuRadar Software" + file->version() );
            h5File_->WriteStrAttrib ( "START_TIME", currentTime.GetTime() );
            h5File_->WriteStrAttrib ( "INSTRUMENT", file->receiver() );
            h5File_->WriteAttrib<int> ( "CHANNELS", file->channel_size(),
                  H5::PredType::NATIVE_INT, H5::DataSpace() );
            h5File_->WriteAttrib<double> ( "SAMPLE_RATE", file->samplerate(),
                  H5::PredType::NATIVE_DOUBLE, H5::DataSpace() );
            h5File_->WriteAttrib<double> ( "BANDWIDTH", file->bandwidth(),
                  H5::PredType::NATIVE_DOUBLE, H5::DataSpace() );
            h5File_->WriteAttrib<int> ( "DECIMATION", file->decimation(),
                  H5::PredType::NATIVE_INT, H5::DataSpace() );
            h5File_->WriteAttrib<double> ( "OUTPUT_RATE", file->outputrate(),
                  H5::PredType::NATIVE_DOUBLE, H5::DataSpace() );
            h5File_->WriteAttrib<double> ( "IPP", file->ipp(),
                  H5::PredType::NATIVE_DOUBLE, H5::DataSpace() );
            h5File_->WriteAttrib<double> ( "RF", file->txcarrier() , 
                  H5::PredType::NATIVE_DOUBLE, H5::DataSpace() );

            for ( int i = 0; i < file->channel_size(); ++i ) {

               h5File_->WriteAttrib<double> ( 
                     "DDC" + lexical_cast<string> ( i ),
                     file->channel(i).frequency(), 
                     H5::PredType::NATIVE_DOUBLE, 
                     H5::DataSpace() 
                     );

               h5File_->WriteAttrib<double> ( 
                     "PHASE" + lexical_cast<string> ( i ),
                     file->channel(i).phase(), 
                     H5::PredType::NATIVE_DOUBLE, 
                     H5::DataSpace() 
                     );
            }

            h5File_->WriteAttrib<int> ( 
                  "SAMPLE_WINDOWS", file->window_size(),
                  H5::PredType::NATIVE_INT, H5::DataSpace()
                  );

            for ( int i = 0; i < file->window_size(); ++i ) {

               // TODO: Window Renaming scheme - 10/19/2010
               // Standardize window naming and add the user-defined
               // window name as a separate attribute.
               string idx = boost::lexical_cast<string> ( i );

               h5File_->WriteAttrib<int> ( 
                     "RxWin"+ idx + "_START", 
                     file->window(i).start(),
                     H5::PredType::NATIVE_INT, H5::DataSpace()
                     );

               h5File_->WriteAttrib<int> ( 
                     "RxWin" + idx + "_STOP", 
                     file->window(i).stop(),
                     H5::PredType::NATIVE_INT, H5::DataSpace()
                     );

               // update gnuradar shared buffer header
               header_->AddWindow( file->window(i).name(), file->window(i).start(), file->window(i).stop() );
            }

            return h5File_;
         }

         public:

         /////////////////////////////////////////////////////////////////////////////
         /////////////////////////////////////////////////////////////////////////////
         Start( zmq::context_t& ctx, PCModelPtr pcModel): GnuRadarCommand( "start" ), pcModel_( pcModel )
         {
            std::string ipaddr = gr_helper::ReadConfigurationFile("status");
            statusServer_ = StatusServerPtr( new network::StatusServer( ctx, ipaddr, pcModel ) );
         }

         /////////////////////////////////////////////////////////////////////////////
         /////////////////////////////////////////////////////////////////////////////
         virtual const gnuradar::ResponseMessage Execute( gnuradar::ControlMessage& msg ){

            std::cout << "GNURADAR: RUN CALLED" << std::endl;

            gnuradar::ResponseMessage response_msg;

            try{
               // reset any existing configuration
               producer_.reset();
               consumer_.reset();
               bufferManager_.reset();
               hdf5_.reset();
               array_.clear();

               gnuradar::File* file = msg.mutable_file();
               gnuradar::RadarParameters* rp = file->mutable_radarparameters();

               // setup shared buffer header to assist in real-time processing 
               header_ = SharedBufferHeaderPtr( new ::xml::SharedBufferHeader(
                        constants::NUM_BUFFERS,
                        rp->bytesperbuffer(),
                        file->samplerate(),
                        file->channel_size(),
                        rp->prisperbuffer(),
                        rp->samplesperbuffer()
                        ));


               // read and parse configuration file->
               GnuRadarSettingsPtr settings = GetSettings( file );

               // create a device to communicate with hardware
               GnuRadarDevicePtr gnuRadarDevice( new GnuRadarDevice( settings ) );

               // make sure we don't have an existing data set
               CheckForExistingFileSet ( file->basefilename() ) ;

               // setup HDF5 attributes and file->set.
               hdf5_ = SetupHDF5( file );

               // setup shared memory buffers
               CreateSharedBuffers( rp->bytesperbuffer() );

               // setup the buffer manager
               bufferManager_ = SynchronizedBufferManagerPtr( 
                     new SynchronizedBufferManager( 
                        array_, constants::NUM_BUFFERS, rp->bytesperbuffer()) );

               // setup table dimensions column = samples per ipp , row = IPP number
               vector<hsize_t> dims;
               dims.push_back( rp->prisperbuffer() );
               dims.push_back ( static_cast<int> ( rp->samplesperpri() * file->channel_size() ) );

               // setup producer thread
               producer_ = gnuradar::ProducerThreadPtr (
                     new ProducerThread ( bufferManager_, gnuRadarDevice ) );

               // flush header information
               header_->Close();

               // setup consumer thread
               consumer_ = gnuradar::ConsumerThreadPtr(
                     new ConsumerThread ( bufferManager_ , header_, hdf5_, dims ) );

               // new model
               pcModel_->Initialize( bufferManager_, producer_, consumer_);

               // start producer thread
               pcModel_->Start();

               response_msg.set_value(gnuradar::ResponseMessage::OK);
               response_msg.set_message("Data collection successfully started.");

               // Start status thread to broadcast status packets to any subscribers.
               if( statusServer_->IsActive() == false )
               {
                  statusServer_->Start();
               }

            }
            catch( std::runtime_error& e ){

               response_msg.set_value(gnuradar::ResponseMessage::ERROR);
               response_msg.set_message(e.what());

            }
            catch( H5::Exception& e ){

               response_msg.set_value(gnuradar::ResponseMessage::ERROR);
               response_msg.set_message(e.getDetailMsg());
            }

            return response_msg;
         }
      };
   };
};

#endif
