/**
 * named_parameters.js
 *
 * Allow passing parameters to a function by name.
 * See docs/system_scripts/named_parameters.txt for more details.
 *
 * @author	tunginobi
 */

Object.prototype.merge = function (obj)
{
	for (var t in this)
	{
		if (t in obj)
		{
			if (typeof obj[t] === "object" && typeof this[t] === "object")
				this[t].merge(obj[t]);
			else
				this[t] = obj[t];
		}
	}
}
