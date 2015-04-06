#ifndef __UNIQUE_NAMER_H__
#define __UNIQUE_NAMER_H__
class UniqueNamer {
private:
	const String prefix;
	int counter;
	Map<String, String> existing_;

	/**
	    Creates a new UniqueNamer. A UniqueNamer issues unique names, keeping
	    track of any previously issued names.

	    @param prefix
	              the prefix to use ('&lt;prefix&gt;&lt;counter&gt;').
	*/
public:
	UniqueNamer ( String prefix_ ) :
		prefix ( prefix_ ),
		counter ( 0 ) {
	}

	/**
	    Gets the new name for the given old name, where if no old name is given a
	    new name will be generated.

	    @param oldName
	              the old name to get the new name for.

	    @return the new name.
	*/
	String getName ( String oldName ) {
		if ( oldName != null && this.existing.containsKey ( oldName ) )
			return this.existing.get ( oldName );

		const String name = this.prefix + this.counter;
		this.counter++;

		if ( oldName != null )
			this.existing.put ( oldName, name );

		return name;
	}

	String getName() {
		return getName ( null );
	}

	Boolean isNamed ( String oldName ) {
		return existing.containsKey ( oldName );
	}

	Map<String, String> existing() {
		return existing_;
	}
};
#endif
