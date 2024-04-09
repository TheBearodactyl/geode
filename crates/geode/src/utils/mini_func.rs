use std::marker::PhantomData;
use std::mem;
use std::ops::Deref;

pub trait MiniFunctionTrait<Ret, Args>
where
    Ret: 'static,
    Args: 'static,
{
    fn call(&self, args: Args) -> Ret;
    fn clone_box(&self) -> Box<dyn MiniFunctionTrait<Ret, Args>>;
}

impl<Ret, Args, F> MiniFunctionTrait<Ret, Args> for F
where
    Ret: 'static,
    Args: 'static,
    F: 'static + Clone + Fn(Args) -> Ret,
{
    fn call(&self, args: Args) -> Ret {
        self(args)
    }

    fn clone_box(&self) -> Box<dyn MiniFunctionTrait<Ret, Args>> {
        Box::new(self.clone())
    }
}

pub struct MiniFunction<Ret, Args>
where
    Ret: 'static,
    Args: 'static,
{
    state: Option<Box<dyn MiniFunctionTrait<Ret, Args>>>,
    _marker: PhantomData<fn(Args) -> Ret>,
}

impl<Ret, Args> MiniFunction<Ret, Args>
where
    Ret: 'static,
    Args: 'static,
{
    pub fn new() -> Self {
        MiniFunction {
            state: None,
            _marker: PhantomData,
        }
    }

    pub fn from_callable<F>(func: F) -> Self
    where
        F: 'static + Fn(Args) -> Ret + Clone,
    {
        MiniFunction {
            state: Some(Box::new(func)),
            _marker: PhantomData,
        }
    }

    pub fn call(&self, args: Args) -> Ret {
        if let Some(state) = &self.state {
            state.call(args)
        } else {
            unsafe { mem::zeroed() }
        }
    }
}

impl<Ret, Args> Clone for MiniFunction<Ret, Args>
where
    Ret: 'static,
    Args: 'static,
{
    fn clone(&self) -> Self {
        MiniFunction {
            state: self.state.as_ref().map(|state| state.clone_box()),
            _marker: PhantomData,
        }
    }
}

impl<Ret, Args> Default for MiniFunction<Ret, Args>
where
    Ret: 'static,
    Args: 'static,
{
    fn default() -> Self {
        Self::new()
    }
}

impl<Ret, Args> Deref for MiniFunction<Ret, Args>
where
    Ret: 'static,
    Args: 'static,
{
    type Target = dyn MiniFunctionTrait<Ret, Args>;

    fn deref(&self) -> &Self::Target {
        unsafe {
            if let Some(state) = &self.state {
                state.deref()
            } else {
                &|_: Args| mem::zeroed()
            }
        }
    }
}
