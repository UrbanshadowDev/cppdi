# CPP-DI
CPP-DI is a lightweight dependency injection header for C++
### Goals
- Straightforward to use
- Single file header
- Does not depend on third party libraries
- Provides dependency injection for binded types
- Provides singleton and factory based instantiations
- Cleans up after itself
- Is not global
- Can be instantiated, sealed and moved around

## Installation
Just copy the `cppdi.h` in your include folder.
## How to use
Include the header in the file you want to use it. Inside the `di` namespace you will find the class `DiContainer`. Interact with this class to configure and retrieve your own classes.
### Example scenario
Considerations:
- We have a class A: `class A`
- We have a class B: `class B`
- We have a class C: `class C`
- Class A has no dependencies `A(){}`
- Class B has no dependencies `B(){}`
- Class C requires an A pointer and a B pointer to be built: `C(A* a, B* b){}`

Regular C++:
```
A a;
B b;
C c(&a, &b);

c.SomeFunction();
```

By using CPP-DI:
```
DI::Container ctnr;

ctnr.Bind<A>();
ctnr.Bind<B>();
ctnr.Bind<C, A, B>();

ctnr.Get<C>()->SomeFunction();
```
The construction details are hidden during the retrieval of the reference of the object we want to call. With correct binding configuration, all classes and dependencies of a program can be built by a single call to `Get`.

Classes with the same dependencies will share instances automatically and everything will be connected as you requested. 

When the container is destroyed it will call the destructor of every instance created by `Get` in reverse order they have been created. You don't have to worry about leaking memory anymore!
### Advanced Usage
See Basic usage first.

Type instantiation can be configured to be generated each time we call `Get`:
```
DI::Container ctnr;
cntr.Bind<A>(DI::Instantiation::factory);
cntr.Bind<B>(DI::Instantiation::factory);
ctnr.Bind<C, A, B>();

A* ex1 = ctnr->Get<A>()
A* ex2 = ctnr->Get<A>()
C* ex3 = ctnr->Get<C>()
ex3->SomeFunction();
```
In this case ex1 and ex2 will be different instances. Also the instance of A generated and injected to ex3 will be different than the other two but since C was configured without parameters (singleton), every time we fetch for C, the same instance of C storing the same instance of A will be retrieved.

Type instantiation can also be configured to provide a specific instance. In the following example, we use this feature to move around the container itself!
```
DI::Container ctnr;
ctnr.Bind<DI::Container>(DI::Instantiation::provided, &ctnr);
ctnr.Bind<Core, DI::Container>();

Core* c = ctnr.Get<Core>();
```
The instance provided will behave like a singleton but the container will not track this instance lifecycle. In this example this is very convenient as it lets the container destroy itself when going out of scope, destroying the core instance. This allows for a very fine grain design of a project lifecycle, allowing controlled destruction of dependency trees by selecting specific instances to be provided.


## Documentation
### di::DiContainer
- `void Bind<T, [D...]>([di::Instantiation], [instance])` This call will bind type `T` constructor with types `D...`. Any type will be correctly resolved if it was previously binded. `D` can be omitted when the constructors have no parameters. `Bind` needs the inversion of control to be respected and assumes all constructor parameters to be pointers. If a type was already binded it will not overwrite the binding. Instantiation can be omitted and is `singleton` by default. Instantiation can be `singleton`, `factory` or `provided`. If the instantiation mode is `provided` will use the value of `instance` to resolve the dependency as singleton. A `provided` instance **will not be tracked for destruction**.
- `T* Get<T>()` This call will get a pointer to an instance of type T from the configured binds. If a type was binded with construction parameters it will try to recursively resolve all parameter types from the configured bindes automatically. If a type is missing will return (or inject) *nullptr* instead.
- `void Seal()` This call will seal the container. Any call to `Bind` after sealing will take no effect and will return silently. A container cannot be unsealed.

## How it works
/ ! \ Warning, dense write-up incoming / ! \

Due to C++ not having a standard reflection library and the decision on how C++ is unable to operate with types themselves, we have to resort to type resolution during compilation. Runtime-wise this is cheating but for the developer looks and behaves just like the real deal.

To use compilation tricks to do the work the code relies in three features of C++:
- Variadic function templates
- Variadic class templates
- Polymorphism of class templates

### Storing type configurations
In the code, the `Ctor` class is defined as a variadic class template with a base class:

```
template<class T, class D...> class Ctor : public Binding
```

This specific template lets differentiate between the type T and all the types (D1, D2, D3...) of the parameters of a constructor of T. By having a base class, all different shapes of the template can be stored by referencing the base class instead, for example, in an array:  
```
Binding** myBinds = new Binding*[10];
```
This way, a configuration of types can be generated in compilation time but its instances managed, stored and used in runtime. The developer can request new templates of `Ctor` by calling the `DI::Container::Bind` function. This will create the template and instantiate the object if it was singleton (it is by default). 

### Building dependency trees
So long the type T has a constructor with the amount and type of D variadic parameters and all these parameters have constructors without parameters, this instantiation might work while calling `Ctor::Build`: 
```
T* obj = new T(new D()...);
```
But of course, in the real world the dependencies of an object might have a variable amount of dependencies themselves. To fix this an extra layer is added, the container itself.

The container is injected into each binding and at the same time stores the collection of all bindings so any specific binding can request the container information and instances of any other binding (including itself, beware of recursion!) required to build itself, thus closing the circle:
```
I* obj = new T(container->Get<D>()...);
```
Remember any D is still a type, so the `DI::Container::Get` function should be templated.
The `DI::Container::Get` function is also be used by the developer to request new types to the container.

### Configuring Instances

If the `Ctor::Build` function succeeded the built instance of the bind will be tracked until the container instance is destroyed. Depending on the type of instancing, the container will behave differently. There are three kinds of instancing currently supported:
- singleton: an instance of this type will be built during binding and will return the same reference for every call to `DI::Container::Get`.
- factory: an instance of this type will not be built during binding and will return a new reference for every call to `DI::Container::Get`. 
- provided: an instance of this type will store the provided reference as will return said reference for every call to `DI::Container::Get`.

### Destroying tracked instances
All `singleton` and `factory` instances will be tracked for destruction. If an instance of a container provided an undeterminate amount of instances and is destroyed, all instances provided will have its destructor called **in the reverse order they have been provided**. This ensures unobstructed destruction or broken references if the bindings and the gets have been properly configured.

Any `provided` references will not be tracked and have to be manually destroyed. They can be manually destroyed by a tracked instance, the container will not care (unless you are destoying the container itself!).


## Contributing
Is something missing? Something can be improved without going against the project goals?
- Open a branch and submit a a pull request!
- Fork it!

### Support
If you really want to support, get me a coffee!

https://ko-fi.com/luiscondesdi

