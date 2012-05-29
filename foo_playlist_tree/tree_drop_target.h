#pragma once

class tree_drop_target : public IDropTarget
{
private:
	int refCount;
	
public:

   playlist_tree_panel * m_panel;
   node                * m_target;


	tree_drop_target( playlist_tree_panel * p )
	{
      m_panel = p;
		refCount = 0;
	}
	
   HRESULT STDMETHODCALLTYPE QueryInterface(REFIID iid,void ** ppvObject)
	{
		if (ppvObject==0) 
		{
			return E_INVALIDARG;
		}
		else if (iid == IID_IUnknown) 
		{
			AddRef();
			*ppvObject = (IUnknown*)this;
			return S_OK;
		}
		else if (iid == IID_IDropTarget) 
		{
			AddRef();
			*ppvObject = (IDropTarget*)this;
			return S_OK;
		}
      return E_NOINTERFACE;
	}
   
	ULONG STDMETHODCALLTYPE AddRef() 
	{
		refCount++;
		return refCount;
	}

	ULONG STDMETHODCALLTYPE Release()
	{
		refCount--;
		return refCount;
	}

	HRESULT STDMETHODCALLTYPE DragEnter( IDataObject * pDataObject, DWORD grfKeyState, POINTL pt, DWORD * pdwEffect);

	HRESULT STDMETHODCALLTYPE DragOver(DWORD grfKeyState, POINTL pt, DWORD * pdwEffect);

	HRESULT STDMETHODCALLTYPE DragLeave( void );
 
	HRESULT STDMETHODCALLTYPE Drop( IDataObject * pDataObject, DWORD grfKeyState, POINTL pt, DWORD * pdwEffect);
};
