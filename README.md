# CPP-DI
CPP-DI is a lightweight dependency injection header for C++
### Goals
- Straightforward to use
- Single file header
- Does not depend on third party libraries
- Provides dependency injection for binded types
- Provides singleton and factory based instantiations
- Cleans up after itself when possible
- Is not global
- Can be instantiated, sealed and moved around

## How to use

### Basic Usage
Considerations:
- We have a class A with a base class BaseA: `class A : public BaseA`
- We have a class B with a base class BaseB: `class B : public BaseB`
- We have a class C with a base class BaseC: `class C : public BaseC`
- Class A has no dependencies `A(){}`
- Class B has no dependencies `B(){}`
- Class C requires a BaseA pointer and a BaseB pointer to be built: `C(BaseA* ba, BaseB* bb){}`

Regular C++:
```
A a;
B b;
C c(&a, &b);

c.SomeFunction();
```

By using CPP-DI:
```
di::DiContainer ctnr;

ctnr.Bind<BaseA, A>();
ctnr.Bind<BaseB, B>();
ctnr.Bind<BaseC, C, BaseA, BaseB>();

ctnr->Get<C>()->SomeFunction();
```
The construction details are hidden during the retrieval of the reference of the object we want to call. With correct binding configuration, all classes and dependencies of a program can be built by a single call to `Get`.

Classes with the same dependencies will share instances automatically and everything will be connected as you requested. 

When the container is destroyed it will call the destructor of every configured class in the reverse order they have been binded. You don't have to worry about leaking memory anymore!
### Advanced Usage
See Basic usage first.

Specific types can be configured to be generated each time we call `Get`:
```
di::DiContainer ctnr;
cntr.Bind<BaseA, A>(di::Instantiation::factory);
cntr.Bind<BaseB, B>(di::Instantiation::factory);
ctnr.Bind<BaseC, C, BaseA, BaseB>();

A* ex1 = ctnr->Get<A>()
A* ex2 = ctnr->Get<A>()

C* ex3 = ctnr->Get<C>()
ex3->SomeFunction();
```
In this case ex1 and ex2 will be different. Also the instance of A generated and injected to ex3 will be different than the other two but since C was configured without parameters (singleton), every time we fetch for C, the same instance of C storing the same instance of A will be retrieved.

The container will not track factory configured instances. In the example scenario, the container will expect class C (singleton) to cleanup for A, B factory allocated. When the container is destroyed it will correctly call C and every singleton configured instance in the reverse order they were configured but will ignore factory configured bindings.  

## How it works
/ ! \ Warning, dense write-up incoming / ! \

Due to C++ not having a standard reflection library and the decision on how C++ is unable to operate with types themselves, we have to resort to type resolution during compilation. Runtime-wise this is cheating but for the developer looks and behaves just like the real deal.

To use compilation tricks to do the work the code relies in three features of C++:
- Variadic function templates
- Variadic class templates
- Polymorphism of class templates

### Storing type configurations
In the code, the `CtorBind` class is defined as a variadic class template with a base class:

```
template<typename I, class T, class D...> class CtorBind : public Binding
```

This specific template lets differentiate between the base class I, the type T and all the types (D1, D2, D3...) of the parameters of a constructor of T. By having a base class, all different shapes of the template can be stored by referencing the base class instead, for example, in an array:  
```
Binding** myBinds = new Binding*[10];
```
This way, a configuration of types can be generated in compilation time but its instances managed, stored and used in runtime. The developer can request new templates of `CtorBind` by calling the `DiContainer::Bind` function. This will create the template and instantiate the object if it was singleton (it is by default). 

### Building dependency trees
So long the type T has a constructor with the amount and type of D variadic parameters and all these parameters have constructors without parameters, this instantiation might work while calling `CtorBind::Build`: 
```
I* obj = new T(new D()...);
```
But of course, in the real world the dependencies of an object might have a variable amount of dependencies themselves. To fix this an extra layer is added, the container itself.

The container is injected into each binding and at the same time stores the collection of all bindings so any specific binding can request the container information and instances of any other binding (including itself, beware of recursion!) required to build itself, thus closing the circle:
```
I* obj = new T(container->Get<D>()...);
```


Remember any D is still a type, so the `DiContainer::Get` function should be templated.
The `DiContainer::Get` function is also be used by the developer to request new types to the container.

If the `CtorBind::Build` function succeeded and the bind itself was configured as a singleton, the first built instance of the bind will be preserved until the container instance is destroyed. Every call to `DiContainer::Get` will retrieve the same reference. If the bind was configured as factory, a new reference will be created for each call to `DiContainer::Get` and no instances will be preserved. Factory configured binds will be no longer tracked by the container and rely on proper destruction by the user.

### Sealing
Once a container has been configured, a developer may choose to seal it. What this means is that specific instance will return silently for each call to `DiContainer::Bind` after the Sealing. This works for heavy factory based scenarios where several sealed instances of `DiContainer` configured in different ways may be moved around or even injected into target types. Be aware `DiContainer` cannot be mocked.

## Contributing
Is something missing? Something can be improved without going against the project goals?
- Post an issue!
- Open a branch and submit a a pull request!
- Fork it!

### Support
If you really want to support, get me a coffee!

https://ko-fi.com/luiscondesdi

