class tree_drop_source : public IDropSource
{
public:
	long refcount;
   node * m_node;
   playlist_tree_panel * m_panel;

	tree_drop_source( playlist_tree_panel * panel, node * n ) : refcount(1) 
   {
      m_panel = panel;
      m_node = n;
   }

	virtual HRESULT STDMETHODCALLTYPE QueryInterface( REFIID iid,  void ** ppvObject )
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
		else if (iid == IID_IDropSource) 
		{
			AddRef();
			*ppvObject = (IDropSource*)this;
			return S_OK;
		}
		else 
		{ 
			return E_NOINTERFACE;
		}
	}
	
	virtual ULONG STDMETHODCALLTYPE AddRef() 
	{
		return InterlockedIncrement(&refcount);
	}

	virtual ULONG STDMETHODCALLTYPE Release()
	{
		return InterlockedDecrement(&refcount);
	}

	virtual HRESULT STDMETHODCALLTYPE QueryContinueDrag( BOOL fEscapePressed, DWORD grfKeyState );

	virtual HRESULT STDMETHODCALLTYPE GiveFeedback( DWORD dwEffect );
};

