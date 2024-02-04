# My Progress Wizard

**Tip**: The name of the project is a wrong-spelling (but I am too lazy to correct it). The correct name is "My Progress Widget".

## Features
- Create a progress widget without writing a basic window
- Thread safe (maybe?)
- Object-Handle managing
- Allow multiple instances
- The UI runs in a different thread so it still work even if the main thread is blocked
- Custom cancel handling
- Extensible custom attributes
- etc...

## How to use
1. Add the .lib file to your project.
2. Include "wizard.user.h" in your project.
3. Use the functions.

**Tip**: The .lib file uses /MT option. If your project is /MD, you can either compile the project yourself or change your project's config.

## Usage
I'm lazy so I don't want to write a help document. Instead, you can view the project "ZExampleProj-DeleteDirectoryContent" to know the basic usage of it.

## Example
A short example to create a simple progress bar:
```
#include <Windows.h>
#pragma comment(lib, "MyProgressWizardLib64.lib")
#include "wizard.user.h"

using namespace std;

int main(int argc, char* argv[])
{
	InitMprgComponent(); // remember to init
	HMPRGOBJ hObj = CreateMprgObject();
	assert(hObj);
	HMPRGWIZ hWiz = CreateMprgWizard(hObj, MPRG_CREATE_PARAMS{
		.cb= sizeof(MPRG_CREATE_PARAMS),
	});
	assert(hWiz);
	OpenMprgWizard(hWiz);

    // do things you want...



	DestroyMprgWizard(hObj, hWiz);
	DeleteMprgObject(hObj);
	return 0;
}
```

## License
The project [My Progress Wizard](#Features) is licensed under [MIT license](./LICENSE.txt). However, **you shouldn't publish it to [CSDN](https://csdn.net/)**.

Author: [shc0743](https://github.com/shc0743)
