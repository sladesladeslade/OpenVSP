//
// This file is released under the terms of the NASA Open Source Agreement (NOSA)
// version 1.3 as detailed in the LICENSE file which accompanies this software.
//

// VehicleMgr.cpp: implementation of the Vehicle Class and Vehicle Mgr Singleton.
//
//////////////////////////////////////////////////////////////////////

#ifdef WIN32
#undef _HAS_STD_BYTE
#define _HAS_STD_BYTE 0
#endif

#include "FileUtil.h"

#include "AdvLinkScreen.h"
#include "AdvLinkVarRenameScreen.h"
#include "AeroStructScreen.h"
#include "AirfoilExportScreen.h"
#include "BEMOptionsScreen.h"
#include "CfdMeshScreen.h"
#include "ClippingScreen.h"
#include "CompGeomScreen.h"
#include "CurveEditScreen.h"
#include "DegenGeomScreen.h"
#include "DesignVarScreen.h"
#include "DXFOptionsScreen.h"
#include "ExportScreen.h"
#include "FeaPartEditScreen.h"
#include "FitModelScreen.h"
#include "IGESOptionsScreen.h"
#include "IGESStructureOptionsScreen.h"
#include "ImportScreen.h"
#include "ManageBackgroundScreen.h"
#include "ManageCORScreen.h"
#include "ManageGeomScreen.h"
#include "ManageMeasureScreen.h"
#include "ManageLightingScreen.h"
#include "ManageTextureScreen.h"
#include "ManageViewScreen.h"
#include "MassPropScreen.h"
#include "MaterialEditScreen.h"
#include "SnapToScreen.h"
#include "ParasiteDragScreen.h"
#include "ParmDebugScreen.h"
#include "ParmLinkScreen.h"
#include "ParmScreen.h"
#include "PreferencesScreen.h"
#include "ProjectionScreen.h"
#include "PSliceScreen.h"
#include "ScreenshotScreen.h"
#include "SetEditorScreen.h"
#include "STEPOptionsScreen.h"
#include "STEPStructureOptionsScreen.h"
#include "STLOptionsScreen.h"
#include "StructScreen.h"
#include "StructAssemblyScreen.h"
#include "SurfaceIntersectionScreen.h"
#include "SVGOptionsScreen.h"
#include "CustomGeomExportScreen.h"
#include "UserParmScreen.h"
#include "VarPresetScreen.h"
#include "VSPAEROPlotScreen.h"
#include "VSPAEROScreen.h"
#include "WaveDragScreen.h"

#define UPDATE_TIME (1.0/30.0)

//==== Constructor ====//
ScreenMgr::ScreenMgr( Vehicle* vPtr )
{
    if ( vPtr )
    {
        m_VehiclePtr = vPtr;
        Init();
    }
    m_UpdateFlag = true;
    MessageBase::Register( string( "ScreenMgr" ) );

    Fl::scheme( "GTK+" );
    Fl::add_timeout( UPDATE_TIME, StaticTimerCB, this );
    Fl::add_handler( GlobalHandler );

    m_NativeFileChooser = NULL;

    m_RunGUI = true;

    m_ShowPlotScreenOnce = false;

}

//==== Destructor ====//
ScreenMgr::~ScreenMgr()
{
    for ( int i = 0 ; i < ( int )m_ScreenVec.size() ; i++ )
    {
        delete m_ScreenVec[i];
    }
    m_ScreenVec.clear();
    MessageMgr::getInstance().UnRegister( this );

    if ( m_NativeFileChooser )
    {
        delete m_NativeFileChooser;
    }
}

//==== Force Update ====//
void ScreenMgr::ForceUpdate()
{
    m_UpdateFlag = false;
    UpdateAllScreens();
}

//==== Timer Callback ====//
void ScreenMgr::TimerCB()
{
    if ( m_UpdateFlag )
    {
        if (m_ShowPlotScreenOnce)
        {
            m_ShowPlotScreenOnce = false;
            m_ScreenVec[vsp::VSP_MAIN_SCREEN]->Show();   //set main screen as "current" before show
            m_ScreenVec[vsp::VSP_VSPAERO_PLOT_SCREEN]->Show();
        }
        m_UpdateFlag = false;
        UpdateAllScreens();
    }

    Fl::repeat_timeout( UPDATE_TIME, StaticTimerCB, this );
}


//==== Set Update Flag ====//
void ScreenMgr::SetUpdateFlag( bool flag )
{
    m_UpdateFlag = flag;
}

//==== Message Callbacks ====//
void ScreenMgr::MessageCallback( const MessageBase* from, const MessageData& data )
{
    if ( data.m_String == string( "UpdateAllScreens" ) )
    {
        SetUpdateFlag( true );
    }
    else if ( data.m_String == string( "VSPAEROSolverMessage" ) )
    {
        VSPAEROScreen* scr = ( VSPAEROScreen* ) m_ScreenVec[vsp::VSP_VSPAERO_SCREEN];
        if ( scr )
        {
            for ( int i = 0; i < (int)data.m_StringVec.size(); i++ )
            {
                scr->AddOutputText( scr->GetDisplay( VSPAERO_SOLVER ), data.m_StringVec[i] );
            }
        }
        // Also dump to aero-structure console.
        AeroStructScreen* asscr = ( AeroStructScreen* ) m_ScreenVec[vsp::VSP_AERO_STRUCT_SCREEN];
        if ( asscr )
        {
            for ( int i = 0; i < (int)data.m_StringVec.size(); i++ )
            {
                asscr->AddOutputText( data.m_StringVec[i] );
            }
        }
    }
    else if ( data.m_String == string( "FEAMessage" ) )
    {
        StructScreen* scr = ( StructScreen* ) m_ScreenVec[vsp::VSP_STRUCT_SCREEN];
        if ( scr )
        {
            for ( int i = 0; i < (int)data.m_StringVec.size(); i++ )
            {
                scr->AddOutputText( data.m_StringVec[i] );
            }
        }
        // Also dump to aero-structure console.
        AeroStructScreen* asscr = ( AeroStructScreen* ) m_ScreenVec[vsp::VSP_AERO_STRUCT_SCREEN];
        if ( asscr )
        {
            for ( int i = 0; i < (int)data.m_StringVec.size(); i++ )
            {
                asscr->AddOutputText( data.m_StringVec[i] );
            }
        }
        // And to structures assembly console.
        StructAssemblyScreen* assyscr = ( StructAssemblyScreen* ) m_ScreenVec[vsp::VSP_STRUCT_ASSEMBLY_SCREEN];
        if ( assyscr )
        {
            for ( int i = 0; i < (int)data.m_StringVec.size(); i++ )
            {
                assyscr->AddOutputText( data.m_StringVec[i] );
            }
        }
    }
    else if ( data.m_String == string( "AeroStructMessage" ) )
    {
        AeroStructScreen* asscr = ( AeroStructScreen* ) m_ScreenVec[vsp::VSP_AERO_STRUCT_SCREEN];
        if ( asscr )
        {
            for ( int i = 0; i < (int)data.m_StringVec.size(); i++ )
            {
                asscr->AddOutputText( data.m_StringVec[i] );
            }
        }
    }
    else if ( data.m_String == string( "CFDMessage" ) )
    {
        CfdMeshScreen* scr = (CfdMeshScreen*)m_ScreenVec[vsp::VSP_CFD_MESH_SCREEN];
        if ( scr )
        {
            for ( int i = 0; i < (int)data.m_StringVec.size(); i++ )
            {
                scr->AddOutputText( data.m_StringVec[i] );
            }
        }
    }
    else if ( data.m_String == string( "SurfIntersectMessage" ) )
    {
        SurfaceIntersectionScreen* scr = (SurfaceIntersectionScreen*)m_ScreenVec[vsp::VSP_SURFACE_INTERSECTION_SCREEN];
        if ( scr )
        {
            for ( int i = 0; i < (int)data.m_StringVec.size(); i++ )
            {
                scr->AddOutputText( data.m_StringVec[i] );
            }
        }
    }
    else if ( data.m_String == string( "Error" ) )
    {
        const char* msg = data.m_StringVec[0].c_str();
        fl_message( "%s", ( char* )msg );
    }
    else if ( data.m_String == string( "CheckCollisionKey" ) )
    {
        SnapTo* snap = VehicleMgr.GetVehicle()->GetSnapToPtr();
        if ( snap )
        {
            if ( Fl::event_alt()  )
                snap->m_CollisionDetection = true;
            else
                snap->m_CollisionDetection = false;
        }
    }
}

//==== Init All Screens ====//
void ScreenMgr::Init()
{
    //==== Build All Screens ====//
    m_ScreenVec.resize( vsp::VSP_NUM_SCREENS );
    m_ScreenVec[vsp::VSP_ADV_LINK_SCREEN] = new AdvLinkScreen( this );
    m_ScreenVec[vsp::VSP_ADV_LINK_VAR_RENAME_SCREEN] = new AdvLinkVarRenameScreen( this );
    m_ScreenVec[vsp::VSP_AERO_STRUCT_SCREEN] = new AeroStructScreen( this );
    m_ScreenVec[vsp::VSP_AIRFOIL_CURVES_EXPORT_SCREEN] = new BezierAirfoilExportScreen( this );
    m_ScreenVec[vsp::VSP_AIRFOIL_POINTS_EXPORT_SCREEN] = new SeligAirfoilExportScreen( this );
    m_ScreenVec[vsp::VSP_BACKGROUND_SCREEN] = new ManageBackgroundScreen( this );
    m_ScreenVec[vsp::VSP_BEM_OPTIONS_SCREEN] = new BEMOptionsScreen( this );
    m_ScreenVec[vsp::VSP_CFD_MESH_SCREEN] = new CfdMeshScreen( this );
    m_ScreenVec[vsp::VSP_CLIPPING_SCREEN] = new ClippingScreen( this );
    m_ScreenVec[vsp::VSP_COMP_GEOM_SCREEN] = new CompGeomScreen( this );
    m_ScreenVec[vsp::VSP_COR_SCREEN] = new ManageCORScreen( this );
    m_ScreenVec[vsp::VSP_CURVE_EDIT_SCREEN] = new CurveEditScreen( this );
    m_ScreenVec[vsp::VSP_DEGEN_GEOM_SCREEN] = new DegenGeomScreen( this );
    m_ScreenVec[vsp::VSP_DESIGN_VAR_SCREEN] = new DesignVarScreen( this );
    m_ScreenVec[vsp::VSP_DXF_OPTIONS_SCREEN] = new DXFOptionsScreen( this);
    m_ScreenVec[vsp::VSP_EXPORT_SCREEN] = new ExportScreen( this );
    m_ScreenVec[vsp::VSP_FEA_PART_EDIT_SCREEN] = new FeaPartEditScreen( this );
    m_ScreenVec[vsp::VSP_FEA_XSEC_SCREEN] = new FeaXSecScreen( this );
    m_ScreenVec[vsp::VSP_FIT_MODEL_SCREEN] = new FitModelScreen( this );
    m_ScreenVec[vsp::VSP_IGES_OPTIONS_SCREEN] = new IGESOptionsScreen( this );
    m_ScreenVec[vsp::VSP_IGES_STRUCTURE_OPTIONS_SCREEN] = new IGESStructureOptionsScreen( this );
    m_ScreenVec[vsp::VSP_EXPORT_CUSTOM_SCRIPT] = new CustomGeomExportScreen( this );
    m_ScreenVec[vsp::VSP_IMPORT_SCREEN] = new ImportScreen( this );
    m_ScreenVec[vsp::VSP_MEASURE_SCREEN] = new ManageMeasureScreen( this );
    m_ScreenVec[vsp::VSP_LIGHTING_SCREEN] = new ManageLightingScreen( this );
    m_ScreenVec[vsp::VSP_MAIN_SCREEN] = new MainVSPScreen( this  );
    m_ScreenVec[vsp::VSP_MANAGE_GEOM_SCREEN] = new ManageGeomScreen( this );
    m_ScreenVec[vsp::VSP_MANAGE_TEXTURE_SCREEN] = new ManageTextureScreen( this );
    m_ScreenVec[vsp::VSP_MASS_PROP_SCREEN] = new MassPropScreen( this );
    m_ScreenVec[vsp::VSP_MATERIAL_EDIT_SCREEN] = new MaterialEditScreen( this );
    m_ScreenVec[vsp::VSP_SNAP_TO_SCREEN] = new SnapToScreen( this );
    m_ScreenVec[vsp::VSP_PARASITE_DRAG_SCREEN] = new ParasiteDragScreen( this );
    m_ScreenVec[vsp::VSP_PARM_DEBUG_SCREEN] = new ParmDebugScreen( this );
    m_ScreenVec[vsp::VSP_PARM_LINK_SCREEN] = new ParmLinkScreen( this );
    m_ScreenVec[vsp::VSP_PARM_SCREEN] = new ParmScreen( this );
    m_ScreenVec[vsp::VSP_PICK_SET_SCREEN] = new PickSetScreen( this );
    m_ScreenVec[vsp::VSP_PREFERENCES_SCREEN] = new PreferencesScreen( this );
    m_ScreenVec[vsp::VSP_PROJECTION_SCREEN] = new ProjectionScreen( this );
    m_ScreenVec[vsp::VSP_PSLICE_SCREEN] = new PSliceScreen( this );
    m_ScreenVec[vsp::VSP_SCREENSHOT_SCREEN] = new ScreenshotScreen( this );
    m_ScreenVec[vsp::VSP_SELECT_FILE_SCREEN] = new SelectFileScreen( this );
    m_ScreenVec[vsp::VSP_SET_EDITOR_SCREEN] = new SetEditorScreen( this );
    m_ScreenVec[vsp::VSP_STEP_OPTIONS_SCREEN] = new STEPOptionsScreen( this );
    m_ScreenVec[vsp::VSP_STEP_STRUCTURE_OPTIONS_SCREEN] = new STEPStructureOptionsScreen( this );
    m_ScreenVec[vsp::VSP_STL_OPTIONS_SCREEN] = new STLOptionsScreen( this );
    m_ScreenVec[vsp::VSP_STRUCT_SCREEN] = new StructScreen( this );
    m_ScreenVec[vsp::VSP_STRUCT_ASSEMBLY_SCREEN] = new StructAssemblyScreen( this );
    m_ScreenVec[vsp::VSP_SURFACE_INTERSECTION_SCREEN] = new SurfaceIntersectionScreen( this );
    m_ScreenVec[vsp::VSP_SVG_OPTIONS_SCREEN] = new SVGOptionsScreen( this );
    m_ScreenVec[vsp::VSP_USER_PARM_SCREEN] = new UserParmScreen( this );
    m_ScreenVec[vsp::VSP_VIEW_SCREEN] = new ManageViewScreen( this );
    m_ScreenVec[vsp::VSP_VAR_PRESET_SCREEN] = new VarPresetScreen( this );
    m_ScreenVec[vsp::VSP_VSPAERO_PLOT_SCREEN] = new VSPAEROPlotScreen( this );
    m_ScreenVec[vsp::VSP_VSPAERO_SCREEN] = new VSPAEROScreen( this );
    m_ScreenVec[vsp::VSP_WAVEDRAG_SCREEN] = new WaveDragScreen( this );
    m_ScreenVec[vsp::VSP_XSEC_SCREEN] = new XSecViewScreen( this );

    m_ScreenVec[vsp::VSP_MAIN_SCREEN]->Show();

    // Set manage geom screen to show up to the main screen as the default.
    int x,y,w,h1,h2;
    x = m_ScreenVec[vsp::VSP_MAIN_SCREEN]->GetFlWindow()->x();
    y = m_ScreenVec[vsp::VSP_MAIN_SCREEN]->GetFlWindow()->y();
    w = m_ScreenVec[vsp::VSP_MAIN_SCREEN]->GetFlWindow()->w();
    h1 = m_ScreenVec[vsp::VSP_MAIN_SCREEN]->GetFlWindow()->h();

    m_ScreenVec[vsp::VSP_MANAGE_GEOM_SCREEN]->GetFlWindow()->position(x+w+5,y);

    h2 = m_ScreenVec[vsp::VSP_XSEC_SCREEN]->GetFlWindow()->h();
    m_ScreenVec[vsp::VSP_XSEC_SCREEN]->GetFlWindow()->position( x + w + 5, y + h1 - h2 );

    x = m_ScreenVec[vsp::VSP_MANAGE_GEOM_SCREEN]->GetFlWindow()->x();
    y = m_ScreenVec[vsp::VSP_MANAGE_GEOM_SCREEN]->GetFlWindow()->y();
    w = m_ScreenVec[vsp::VSP_MANAGE_GEOM_SCREEN]->GetFlWindow()->w();

    VSP_Window::SetGeomX( x + w );
    VSP_Window::SetGeomY( y );

    for ( int i = 0 ; i < ( int )m_ScreenVec.size() ; i++ )
    {
        if( i != vsp::VSP_MAIN_SCREEN && i != vsp::VSP_COR_SCREEN )
        {
            m_ScreenVec[i]->GetFlWindow()->set_non_modal();
        }
    }

    // Show() after setting non_modal, as modality can not change if window shown.
    m_ScreenVec[vsp::VSP_MANAGE_GEOM_SCREEN]->Show();
}

//==== Update All Displayed Screens ====//
void ScreenMgr::UpdateAllScreens()
{
//static int last_tics = timeGetTime();
//int del_tics = timeGetTime() - last_tics;
//last_tics = timeGetTime();
//printf("Update Screens %d\n",  del_tics );
    for ( int i = 0 ; i < ( int )m_ScreenVec.size() ; i++ )
    {
        //===== Force Update Of ManageGeomScreen ====//
        if ( m_ScreenVec[i]->IsShown() || (i == vsp::VSP_MANAGE_GEOM_SCREEN) )
        {
            m_ScreenVec[i]->Update();
        }
    }
}

//==== Show Screen ====//
void ScreenMgr::ShowScreen( int id )
{
    if ( id >= 0 && id < vsp::VSP_NUM_SCREENS )
    {
        m_ScreenVec[id]->Show();
    }
}

//==== Hide Screen ====//
void ScreenMgr::HideScreen( int id )
{
    if ( id >= 0 && id < vsp::VSP_NUM_SCREENS )
    {
        m_ScreenVec[id]->Hide();
    }
}

//==== Get Current Geometry To Edit ====//
Geom* ScreenMgr::GetCurrGeom()
{
    vector< Geom* > select_vec = GetVehiclePtr()->GetActiveGeomPtrVec();

    if ( select_vec.size() != 1 )
    {
        return NULL;
    }
    return select_vec[0];
}

VspScreen * ScreenMgr::GetScreen( int id )
{
    if( id >= 0 && id < vsp::VSP_NUM_SCREENS )
    {
        return m_ScreenVec[id];
    }
    // Should not reach here.
    assert( false );
    return NULL;
}

//==== Create Message Pop-Up ====//
void MessageBox( void * data )
{
    fl_message( "%s", ( char* )data );
}

string ScreenMgr::FileChooser( const string &title, const string &filter, int mode, const string &dir )
{
    Fl_Preferences prefs( Fl_Preferences::USER, "openvsp.org", "VSP" );
    Fl_Preferences app( prefs, "Application" );

    int fc_type = vsp::FC_OPENVSP;
    app.get( "File_Chooser", fc_type, vsp::FC_OPENVSP );

    if ( fc_type == vsp::FC_OPENVSP )
    {
        SelectFileScreen * sfc = ( SelectFileScreen * ) m_ScreenVec[ vsp::VSP_SELECT_FILE_SCREEN ];
        return sfc->FileChooser( title, filter, mode, dir );
    }
    else // FC_NATIVE
    {
        return NativeFileChooser( title, filter, mode, dir );
    }
}

string ScreenMgr::NativeFileChooser( const string &title, const string &filter, int mode, const string &dir )
{
    if ( !m_NativeFileChooser )
    {
        m_NativeFileChooser = new Fl_Native_File_Chooser();
    }

    if ( mode == vsp::OPEN )
    {
        m_NativeFileChooser->type( Fl_Native_File_Chooser::BROWSE_FILE );
        m_NativeFileChooser->options( Fl_Native_File_Chooser::PREVIEW );
    }
    else
    {
        m_NativeFileChooser->type( Fl_Native_File_Chooser::BROWSE_SAVE_FILE );
        m_NativeFileChooser->options( Fl_Native_File_Chooser::NEW_FOLDER | Fl_Native_File_Chooser::SAVEAS_CONFIRM | Fl_Native_File_Chooser::USE_FILTER_EXT );
    }

    if ( !dir.empty() )
    {
        m_NativeFileChooser->directory( dir.c_str() );
    }

    bool forceext = true;
    if ( filter.find( ',' ) != string::npos ) // A comma was found
    {
        forceext = false;
    }

    m_NativeFileChooser->title( title.c_str() );
    m_NativeFileChooser->filter( filter.c_str() );

    string fname;
    switch ( m_NativeFileChooser->show() )
    {
        case -1:
            printf( "ERROR: %s\n", m_NativeFileChooser->errmsg() );
            break;
        case 1:
            // Cancel
            break;
        default:
            fname = string( m_NativeFileChooser->filename() );

            if ( forceext )
            {
                ::EnforceFilter( fname, filter );
            }

            break;
    }

    return fname;
}

//==== Create Pop-Up Message Window ====//
void ScreenMgr::Alert( const char * message )
{
    Fl::awake( MessageBox, ( void* )message );
}

int ScreenMgr::GlobalHandler(int event)
{
    if (Fl::event()==FL_SHORTCUT && Fl::event_key()==FL_Escape)
    {
        Vehicle* vPtr = VehicleMgr.GetVehicle();
        if ( vPtr )
        {
            vector< string > none;
            vPtr->SetActiveGeomVec( none );
            MessageMgr::getInstance().Send( "ScreenMgr", "UpdateAllScreens" );
        }
        return 1;
    }
    return 0;
}
